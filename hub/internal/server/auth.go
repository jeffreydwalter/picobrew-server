package server

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/hmac"
	"crypto/pbkdf2"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

type vault struct{ aead cipher.AEAD }

func openVault(dir string) (*vault, error) {
	path := filepath.Join(dir, "master.key")
	key, err := os.ReadFile(path)
	if errors.Is(err, os.ErrNotExist) {
		key = make([]byte, 32)
		if _, err = rand.Read(key); err != nil {
			return nil, err
		}
		var f *os.File
		f, err = os.OpenFile(path, os.O_CREATE|os.O_EXCL|os.O_WRONLY, 0600)
		if err != nil {
			return nil, err
		}
		_, err = f.Write(key)
		ce := f.Close()
		if err == nil {
			err = ce
		}
	}
	if err != nil {
		return nil, err
	}
	if len(key) != 32 {
		return nil, errors.New("master.key must contain exactly 32 bytes")
	}
	if err = os.Chmod(path, 0600); err != nil {
		return nil, err
	}
	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}
	aead, err := cipher.NewGCM(block)
	return &vault{aead}, err
}
func (v *vault) seal(id, value string) []byte {
	nonce := make([]byte, v.aead.NonceSize())
	if _, err := rand.Read(nonce); err != nil {
		panic(err)
	}
	return v.aead.Seal(nonce, nonce, []byte(value), []byte(id))
}
func (v *vault) open(id string, value []byte) (string, error) {
	n := v.aead.NonceSize()
	if len(value) < n {
		return "", errors.New("invalid encrypted credential")
	}
	p, e := v.aead.Open(nil, value[:n], value[n:], []byte(id))
	return string(p), e
}
func randomID() string {
	p := make([]byte, 16)
	if _, err := rand.Read(p); err != nil {
		panic(err)
	}
	return hex.EncodeToString(p)
}
func secretToken() string {
	p := make([]byte, 32)
	if _, err := rand.Read(p); err != nil {
		panic(err)
	}
	return base64.RawURLEncoding.EncodeToString(p)
}

type passwordRecord struct {
	Salt string `json:"salt"`
	Hash string `json:"hash"`
}

func hashPassword(password string) (passwordRecord, error) {
	salt := secretToken()
	hash, err := pbkdf2.Key(sha256.New, password, []byte(salt), 210000, 32)
	return passwordRecord{salt, hex.EncodeToString(hash)}, err
}
func matchesPassword(password string, r passwordRecord) bool {
	hash, err := pbkdf2.Key(sha256.New, password, []byte(r.Salt), 210000, 32)
	if err != nil {
		return false
	}
	return hmac.Equal([]byte(hex.EncodeToString(hash)), []byte(r.Hash))
}

type session struct {
	CSRF    string
	Expires time.Time
}
type authState struct {
	mu         sync.Mutex
	sessions   map[string]session
	setupToken string
	password   passwordRecord
	lastLogin  map[string][]time.Time
}

func (s *Server) authStatus(w http.ResponseWriter, r *http.Request) {
	current, a := s.currentSession(r)
	s.auth.mu.Lock()
	setup := s.auth.password.Hash == "" && !s.cfg.Demo
	s.auth.mu.Unlock()
	out := map[string]any{"authenticated": a, "setup_required": setup, "version": Version, "demo": s.cfg.Demo}
	if a {
		out["csrf_token"] = current.CSRF
	}
	s.json(w, 200, out)
}
func (s *Server) authenticated(r *http.Request) bool {
	_, ok := s.currentSession(r)
	return ok
}
func (s *Server) currentSession(r *http.Request) (session, bool) {
	if s.cfg.Demo {
		return session{CSRF: "demo"}, true
	}
	cookie, err := r.Cookie("pico_hub_session")
	if err != nil {
		return session{}, false
	}
	s.auth.mu.Lock()
	defer s.auth.mu.Unlock()
	v, ok := s.auth.sessions[cookie.Value]
	if !ok {
		return session{}, false
	}
	if time.Now().After(v.Expires) {
		delete(s.auth.sessions, cookie.Value)
		return session{}, false
	}
	return v, true
}
func (s *Server) sameOrigin(r *http.Request) bool {
	if origin := r.Header.Get("Origin"); origin != "" {
		u, e := url.Parse(origin)
		return e == nil && strings.EqualFold(u.Host, r.Host) && (u.Scheme == "http" || u.Scheme == "https")
	}
	return true
}
func (s *Server) authenticate(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		current, valid := s.currentSession(r)
		if !valid {
			s.fail(w, 401, "unauthorized", "Sign in to continue")
			return
		}
		if r.Method != "GET" && r.Method != "HEAD" && r.Method != "OPTIONS" {
			if !s.sameOrigin(r) {
				s.fail(w, 403, "origin", "Cross-origin requests are not allowed")
				return
			}
			if current.CSRF == "" || !hmac.Equal([]byte(current.CSRF), []byte(r.Header.Get("X-CSRF-Token"))) {
				s.fail(w, 403, "csrf", "Refresh your session before retrying")
				return
			}
		}
		next.ServeHTTP(w, r)
	})
}
func (s *Server) login(w http.ResponseWriter, r *http.Request) {
	if s.cfg.Demo {
		s.fail(w, 409, "demo", "Demo mode has no owner account")
		return
	}
	if !s.sameOrigin(r) {
		s.fail(w, 403, "origin", "Invalid request origin")
		return
	}
	var in struct {
		Password string `json:"password"`
		Token    string `json:"token"`
	}
	if !s.decode(w, r, &in) {
		return
	}
	s.auth.mu.Lock()
	defer s.auth.mu.Unlock()
	now := time.Now()
	address, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		address = r.RemoteAddr
	}
	// Do not trust forwarding headers supplied by arbitrary clients. A reverse
	// proxy can enforce a finer per-client limit in front of this shared limit.
	for peer, attempts := range s.auth.lastLogin {
		if len(attempts) == 0 || now.Sub(attempts[len(attempts)-1]) >= time.Minute {
			delete(s.auth.lastLogin, peer)
		}
	}
	if len(s.auth.lastLogin) >= 4096 && s.auth.lastLogin[address] == nil {
		s.fail(w, 429, "rate_limit", "Too many attempts; try again in a minute")
		return
	}
	recent := s.auth.lastLogin[address]
	kept := recent[:0]
	for _, at := range recent {
		if now.Sub(at) < time.Minute {
			kept = append(kept, at)
		}
	}
	if len(kept) >= 10 {
		s.fail(w, 429, "rate_limit", "Too many attempts; try again in a minute")
		return
	}
	s.auth.lastLogin[address] = append(kept, now)
	if len(in.Password) > 256 {
		s.fail(w, 400, "password", "Passwords must not exceed 256 characters")
		return
	}
	if strings.HasSuffix(r.URL.Path, "/setup") {
		if s.auth.password.Hash != "" {
			s.fail(w, 409, "configured", "The owner account is already configured")
			return
		}
		if !hmac.Equal([]byte(in.Token), []byte(s.auth.setupToken)) {
			s.fail(w, 401, "setup_token", "Use the setup token printed by the server")
			return
		}
		if len(in.Password) < 12 || len(in.Password) > 256 {
			s.fail(w, 400, "password", "Use a password between 12 and 256 characters")
			return
		}
		record, e := hashPassword(in.Password)
		if e != nil {
			s.internal(w, e)
			return
		}
		if _, e = s.db.Put("config", "password", record, 0); e != nil {
			s.internal(w, e)
			return
		}
		s.auth.password = record
		s.auth.setupToken = ""
	} else if s.auth.password.Hash == "" || !matchesPassword(in.Password, s.auth.password) {
		s.fail(w, 401, "credentials", "Incorrect password")
		return
	}
	id := secretToken()
	if previous, err := r.Cookie("pico_hub_session"); err == nil {
		delete(s.auth.sessions, previous.Value)
	}
	s.auth.sessions[id] = session{CSRF: secretToken(), Expires: now.Add(24 * time.Hour)}
	for k, v := range s.auth.sessions {
		if now.After(v.Expires) {
			delete(s.auth.sessions, k)
		}
	}
	http.SetCookie(w, &http.Cookie{Name: "pico_hub_session", Value: id, Path: "/", HttpOnly: true, Secure: s.cfg.SecureCookies, SameSite: http.SameSiteStrictMode, MaxAge: 86400})
	s.json(w, 200, map[string]bool{"authenticated": true})
}
func (s *Server) logout(w http.ResponseWriter, r *http.Request) {
	if c, e := r.Cookie("pico_hub_session"); e == nil {
		s.auth.mu.Lock()
		delete(s.auth.sessions, c.Value)
		s.auth.mu.Unlock()
	}
	http.SetCookie(w, &http.Cookie{Name: "pico_hub_session", Path: "/", HttpOnly: true, Secure: s.cfg.SecureCookies, SameSite: http.SameSiteStrictMode, MaxAge: -1})
	w.WriteHeader(204)
}
func (s *Server) saveSecret(id, value string) error {
	return s.db.SaveSecret(id, s.vault.seal(id, value))
}
func (s *Server) readSecret(id string) (string, error) {
	p, e := s.db.GetSecret(id)
	if e != nil {
		return "", e
	}
	return s.vault.open(id, p)
}
func (s *Server) initAuth() error {
	s.auth = authState{sessions: map[string]session{}, lastLogin: map[string][]time.Time{}}
	_, err := s.db.Get("config", "password", &s.auth.password)
	if err != nil && !errors.Is(err, store.ErrNotFound) {
		return err
	}
	if errors.Is(err, store.ErrNotFound) && s.cfg.Password != "" && !s.cfg.Demo {
		if len(s.cfg.Password) < 12 || len(s.cfg.Password) > 256 {
			return fmt.Errorf("HUB_PASSWORD must contain 12 to 256 characters")
		}
		p, e := hashPassword(s.cfg.Password)
		if e != nil {
			return e
		}
		if _, e = s.db.Put("config", "password", p, 0); e != nil {
			return e
		}
		s.auth.password = p
	}
	if s.auth.password.Hash == "" && !s.cfg.Demo {
		s.auth.setupToken = secretToken()
		s.cfg.Logger.Info("Complete owner setup in the browser", "setup_token", s.auth.setupToken)
	}
	return nil
}
