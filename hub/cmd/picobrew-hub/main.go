package main

import (
	"context"
	"errors"
	"flag"
	"log/slog"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/server"
)

func main() {
	listen := flag.String("listen", "127.0.0.1:8080", "HTTP listen address")
	data := flag.String("data", "data", "Persistent data directory")
	web := flag.String("web", "web/dist", "Built web assets directory")
	demo := flag.Bool("demo", false, "Use local simulation without hardware access")
	secure := flag.Bool("secure-cookies", false, "Require HTTPS for owner cookies (enable behind TLS proxy)")
	flag.Parse()
	if *demo {
		host, _, err := net.SplitHostPort(*listen)
		if err != nil || (host != "127.0.0.1" && host != "localhost" && host != "::1") {
			slog.Error("Demo must bind to loopback; run authenticated mode for network deployment")
			os.Exit(1)
		}
	}
	app, e := server.New(server.Config{DataDir: *data, WebDir: *web, Password: os.Getenv("HUB_PASSWORD"), Demo: *demo, SecureCookies: *secure})
	if e != nil {
		slog.Error("Startup failed", "error", e)
		os.Exit(1)
	}
	httpServer := &http.Server{Addr: *listen, Handler: app, ReadHeaderTimeout: 5 * time.Second, ReadTimeout: 30 * time.Second, IdleTimeout: 120 * time.Second, MaxHeaderBytes: 16 << 10}
	listener, e := net.Listen("tcp", *listen)
	if e != nil {
		slog.Error("Could not listen", "error", e)
		_ = app.Close()
		os.Exit(1)
	}
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	done := make(chan error, 1)
	exitCode := 0
	go func() {
		slog.Info("PicoBrew Hub ready", "address", listener.Addr().String(), "version", server.Version, "simulation", *demo)
		done <- httpServer.Serve(listener)
	}()
	select {
	case e = <-done:
		if !errors.Is(e, http.ErrServerClosed) {
			slog.Error("Server failed", "error", e)
			exitCode = 1
		}
	case <-ctx.Done():
		shutdown, cancel := context.WithTimeout(context.Background(), 15*time.Second)
		e = httpServer.Shutdown(shutdown)
		cancel()
		if e != nil {
			_ = httpServer.Close()
		}
	}
	if e = app.Close(); e != nil {
		slog.Error("Close failed", "error", e)
		exitCode = 1
	}
	if exitCode != 0 {
		os.Exit(exitCode)
	}
}
