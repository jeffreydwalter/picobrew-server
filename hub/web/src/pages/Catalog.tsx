import { useEffect, useState } from 'react'
import {
  ArrowDownToLine,
  ArrowLeft,
  ArrowRight,
  BookOpen,
  Copy,
  Library,
  Search,
  Wheat,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, download, errorMessage } from '../lib/api'
import { catalogAssetURL, externalURL, kindLabel } from '../lib/programs'
import type { CatalogDetail, CatalogPage, CatalogReferences, CatalogSource, Recipe } from '../types'
import {
  Badge,
  Button,
  Empty,
  ErrorBanner,
  Field,
  Loading,
  Modal,
  PageTitle,
  SearchBox,
  SectionHeading,
} from '../components/ui'
import { ProgramTable, Provenance } from '../components/Program'

const modelNames: Record<string, string> = {
  pico: 'Pico',
  'pico-s': 'Pico S',
  'pico-pro': 'Pico Pro',
  'pico-c': 'Pico C',
  z: 'Pico Z',
  zymatic: 'Zymatic',
}
const displayModel = (value: string) => modelNames[value] || value

export default function Catalog({ id }: { id?: string }) {
  return id ? <CatalogEntry id={id} /> : <CatalogBrowse />
}
function CatalogBrowse() {
  const hub = useHub()
  const params = new URLSearchParams(location.hash.split('?')[1] || '')
  const [query, setQuery] = useState(params.get('q') || '')
  const [debounced, setDebounced] = useState(query)
  const [source, setSource] = useState(params.get('source') || '')
  const [kind, setKind] = useState(params.get('kind') || '')
  const [model, setModel] = useState(params.get('model') || '')
  const [style, setStyle] = useState(params.get('style') || '')
  const [offset, setOffset] = useState(0)
  const [sourcesOpen, setSourcesOpen] = useState(false)
  const [referencesOpen, setReferencesOpen] = useState(false)
  useEffect(() => {
    const timer = setTimeout(() => setDebounced(query), 200)
    return () => clearTimeout(timer)
  }, [query])
  const filters = new URLSearchParams({
    q: debounced,
    source,
    kind,
    model,
    style,
    offset: String(offset),
    limit: '24',
  })
  const resource = useResource<CatalogPage>(`/catalog?${filters}`)
  function change(update: (value: string) => void, value: string) {
    setOffset(0)
    update(value)
  }
  useEffect(() => {
    const current = new URLSearchParams({ q: debounced, source, kind, model, style })
    for (const key of [...current.keys()]) if (!current.get(key)) current.delete(key)
    history.replaceState(null, '', `#/catalog${current.size ? '?' + current : ''}`)
  }, [debounced, source, kind, model, style])
  const page = resource.data
  return (
    <>
      <PageTitle
        eyebrow="FROM THE COMMUNITY ARCHIVES"
        title="A whole library to explore."
        description="Browse recipes, original PicoPak programs and cleaning routines. Copy a favorite into your library to edit it and prepare a transfer."
        actions={
          <Button variant="secondary" onClick={() => hub.navigate('/recipes')}>
            <BookOpen size={16} />
            Your library
          </Button>
        }
      />
      <div className="catalog-intro panel">
        <div>
          <Library size={27} />
          <div>
            <strong>Bundled with your Hub</strong>
            <p>Recipes and illustrations stay available without the old PicoBrew services.</p>
          </div>
        </div>
        <div className="inline-actions">
          <Button variant="ghost" onClick={() => setSourcesOpen(true)}>
            Sources & credits
          </Button>
          <Button variant="secondary" onClick={() => setReferencesOpen(true)}>
            Machine guides & artwork
          </Button>
        </div>
      </div>
      <div className="catalog-filters panel">
        <SearchBox
          value={query}
          onChange={(value) => change(setQuery, value)}
          placeholder="Search community recipes and programs"
        />
        <div className="form-grid quarters">
          <Field label="Catalogue type">
            <select value={kind} onChange={(event) => change(setKind, event.target.value)}>
              <option value="">All recipes & programs</option>
              <option value="recipe">Ingredient recipes</option>
              <option value="native-program">Machine programs</option>
              <option value="maintenance">Cleaning programs</option>
            </select>
          </Field>
          <Field label="Source archive">
            <select value={source} onChange={(event) => change(setSource, event.target.value)}>
              <option value="">All archives</option>
              {page?.sources.map((item) => (
                <option key={item.id} value={item.id}>
                  {item.name}
                </option>
              ))}
            </select>
          </Field>
          <Field label="Source machine">
            <select value={model} onChange={(event) => change(setModel, event.target.value)}>
              <option value="">All machines</option>
              {Object.entries(modelNames)
                .filter(([key]) => key !== 'pico')
                .map(([key, name]) => (
                  <option key={key} value={key}>
                    {name}
                  </option>
                ))}
            </select>
          </Field>
          <Field label="Beer style">
            <select value={style} onChange={(event) => change(setStyle, event.target.value)}>
              <option value="">All styles</option>
              {page?.styles.map((value) => (
                <option key={value}>{value}</option>
              ))}
            </select>
          </Field>
        </div>
      </div>
      <ErrorBanner message={resource.error} retry={resource.reload} />
      {resource.loading ? (
        <Loading label="Opening the community catalogue…" />
      ) : (
        page && (
          <>
            <div className="list-toolbar">
              <span className="muted" role="status">
                {page.total.toLocaleString()} matching entries
              </span>
              <span className="help-text">
                Machine programs contain brewing steps; ingredient recipes include a grain bill.
              </span>
            </div>
            {page.items.length ? (
              <div className="recipe-grid catalog-grid">
                {page.items.map((entry, index) => (
                  <article className="recipe-card catalog-card" key={entry.id}>
                    <div className={`recipe-art tone-${index % 3}`}>
                      {catalogAssetURL(entry.image_url) ? (
                        <img
                          src={entry.image_url}
                          alt={`${entry.style || 'Beer'} illustration`}
                          loading="lazy"
                        />
                      ) : (
                        <Wheat size={58} strokeWidth={1.1} />
                      )}
                      <span>{entry.style || kindLabel(entry.kind)}</span>
                      {entry.featured && (
                        <strong className="catalog-featured">Starter program</strong>
                      )}
                    </div>
                    <div className="recipe-card-content">
                      <Badge
                        tone={
                          entry.kind === 'recipe'
                            ? 'green'
                            : entry.kind === 'maintenance'
                              ? 'neutral'
                              : 'copper'
                        }
                      >
                        {kindLabel(entry.kind)}
                      </Badge>
                      <h3>
                        <button
                          onClick={() => hub.navigate(`/catalog/${encodeURIComponent(entry.id)}`)}
                        >
                          {entry.name}
                        </button>
                      </h3>
                      <p>
                        {entry.description ||
                          (entry.kind === 'native-program'
                            ? 'Original temperature, timing and compartment sequence. Ingredient quantities are not included.'
                            : entry.kind === 'maintenance'
                              ? 'An archived cleaning routine for its source machine.'
                              : 'An archived recipe ready to copy and review.')}
                      </p>
                      <div className="recipe-meta">
                        {entry.abv != null && <span>{entry.abv}% ABV</span>}
                        {entry.ibu != null && <span>{entry.ibu} IBU</span>}
                        <span>{(entry.models || []).map(displayModel).join(' · ')}</span>
                      </div>
                      <footer>
                        <small>
                          {entry.can_deploy ? 'Review before transfer' : 'Reference & planning'}
                        </small>
                        <button
                          onClick={() => hub.navigate(`/catalog/${encodeURIComponent(entry.id)}`)}
                        >
                          View details
                          <ArrowRight size={15} />
                        </button>
                      </footer>
                    </div>
                  </article>
                ))}
              </div>
            ) : (
              <div className="panel">
                <Empty
                  icon={<Search size={26} />}
                  title="No entries match these filters."
                  description="Try a different name, archive or machine."
                  action={
                    <Button
                      variant="secondary"
                      onClick={() => {
                        setQuery('')
                        setSource('')
                        setKind('')
                        setModel('')
                        setStyle('')
                        setOffset(0)
                      }}
                    >
                      Clear filters
                    </Button>
                  }
                />
              </div>
            )}
            <div className="catalog-pagination">
              <Button
                variant="secondary"
                disabled={offset === 0}
                onClick={() => setOffset(Math.max(0, offset - page.limit))}
              >
                <ArrowLeft size={16} />
                Previous
              </Button>
              <span>
                {page.total
                  ? `${offset + 1}–${Math.min(offset + page.items.length, page.total)} of ${page.total.toLocaleString()}`
                  : 'No results'}
              </span>
              <Button
                variant="secondary"
                disabled={offset + page.items.length >= page.total}
                onClick={() => setOffset(offset + page.limit)}
              >
                Next
                <ArrowRight size={16} />
              </Button>
            </div>
          </>
        )
      )}
      {sourcesOpen && (
        <Sources sources={page?.sources || []} onClose={() => setSourcesOpen(false)} />
      )}
      {referencesOpen && <References model={model} onClose={() => setReferencesOpen(false)} />}
    </>
  )
}
function CatalogEntry({ id }: { id: string }) {
  const hub = useHub()
  const resource = useResource<CatalogDetail>(`/catalog/${encodeURIComponent(id)}`)
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const entry = resource.data
  async function copy() {
    setBusy(true)
    setError('')
    try {
      const recipe = await api<Recipe>(`/catalog/${encodeURIComponent(id)}/copy`, {
        method: 'POST',
        body: {},
      })
      await hub.refresh()
      hub.notify('Editable copy added to your library.')
      hub.navigate(`/recipes/${recipe.id}`)
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  if (resource.loading) return <Loading label="Opening the source record…" />
  if (!entry) return <ErrorBanner message={resource.error} retry={resource.reload} />
  const program = entry.native_program || entry.recipe?.native_program
  return (
    <>
      <PageTitle
        back={() => hub.navigate('/catalog')}
        eyebrow={kindLabel(entry.kind).toUpperCase()}
        title={entry.name}
        description={
          entry.description ||
          'An original community archive record. Your editable library copy keeps its source attribution.'
        }
        actions={
          <Button disabled={busy || !entry.can_copy} onClick={() => void copy()}>
            <Copy size={16} />
            {busy ? 'Copying…' : 'Copy to my library'}
          </Button>
        }
      />
      <ErrorBanner message={error || resource.error} />
      <div className="editor-layout catalog-detail">
        <div className="editor-main">
          <section className="panel">
            <SectionHeading
              title={program ? 'The original machine schedule' : 'The original recipe'}
              description={
                entry.kind === 'recipe'
                  ? 'Review this source recipe before adapting it to your equipment.'
                  : entry.kind === 'maintenance'
                    ? 'Follow this cleaning program’s preparation instructions for its source machine. Cleaning routines are kept separate from beer recipes.'
                    : 'This program has no ingredient quantities or measured batch volume. Use its matching PicoPak or a separately reviewed ingredient plan.'
              }
            />
            {program ? (
              <ProgramTable program={program} />
            ) : (
              entry.recipe && (
                <>
                  <p className="catalog-recipe-meta">
                    {entry.recipe.batch_liters} L · {entry.recipe.efficiency}% efficiency ·{' '}
                    {entry.recipe.ingredients?.length || 0} ingredients
                  </p>
                  <div className="table-scroll">
                    <table>
                      <thead>
                        <tr>
                          <th>Ingredient</th>
                          <th>Amount</th>
                          <th>Use</th>
                        </tr>
                      </thead>
                      <tbody>
                        {entry.recipe.ingredients?.map((ingredient, index) => (
                          <tr key={index}>
                            <td>{ingredient.name}</td>
                            <td>
                              {ingredient.amount} {ingredient.unit}
                            </td>
                            <td>{ingredient.use}</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                  <ol className="catalog-schedule">
                    {entry.recipe.steps?.map((step, index) => (
                      <li key={index}>
                        <strong>{step.name}</strong>
                        <span>
                          {step.target_c} °C · {step.minutes} min
                          {step.drain_minutes ? ` · drain ${step.drain_minutes} min` : ''}
                        </span>
                      </li>
                    ))}
                  </ol>
                </>
              )
            )}
            {!entry.can_copy && (
              <p className="warning-note">
                This source record needs further conversion before it can become an editable recipe.
                The original remains available below.
              </p>
            )}
          </section>
          {(entry.notes || entry.warnings?.length > 0) && (
            <section className="panel">
              <SectionHeading title="Source notes" />
              {entry.notes && <p className="preserve-lines">{entry.notes}</p>}
              {entry.warnings?.length > 0 && (
                <ul className="warning-list">
                  {entry.warnings.map((warning, index) => (
                    <li key={index}>{warning}</li>
                  ))}
                </ul>
              )}
            </section>
          )}
          <section className="panel">
            <SectionHeading
              title="Original records"
              description={`${entry.source_record_count} source record${entry.source_record_count === 1 ? '' : 's'} retained. Exact duplicate program variants are grouped in the catalogue.`}
            />
            <div className="catalog-originals">
              {entry.source_records.map((record) => (
                <Button
                  key={record.id}
                  variant="secondary"
                  onClick={async () => {
                    try {
                      await download(
                        `/catalog/${encodeURIComponent(id)}/source?record=${encodeURIComponent(record.id)}`,
                        `${id}-${record.id}.json`
                      )
                    } catch (error) {
                      setError(errorMessage(error))
                    }
                  }}
                >
                  <ArrowDownToLine size={14} />
                  Record {record.id}
                </Button>
              ))}
            </div>
            <Provenance value={entry.provenance} />
          </section>
        </div>
        <aside className="editor-aside">
          <div className="recipe-summary">
            {catalogAssetURL(entry.image_url) ? (
              <img
                className="catalog-detail-image"
                src={entry.image_url}
                alt={`${entry.style || 'Beer'} illustration`}
              />
            ) : (
              <Wheat size={50} />
            )}
            <Badge tone="copper">{kindLabel(entry.kind)}</Badge>
            <h3>{entry.style || entry.name}</h3>
            {entry.image_url && (
              <p>
                Archive illustration
                {entry.image_kind ? ` · ${entry.image_kind.replaceAll('-', ' ')}` : ''}
              </p>
            )}
            <dl>
              <div>
                <dt>Source machine</dt>
                <dd>{displayModel(entry.source_model)}</dd>
              </div>
              {entry.author && (
                <div>
                  <dt>Author</dt>
                  <dd>{entry.author}</dd>
                </div>
              )}
              {entry.abv != null && (
                <div>
                  <dt>ABV</dt>
                  <dd>{entry.abv}%</dd>
                </div>
              )}
              {entry.ibu != null && (
                <div>
                  <dt>IBU</dt>
                  <dd>{entry.ibu}</dd>
                </div>
              )}
            </dl>
            <p>
              {entry.can_deploy
                ? `Transfer support: ${entry.deploy_models.map(displayModel).join(', ')}. The preview checks the connected machine and its available slots.`
                : 'Available for reference and planning. Transfer compatibility is checked when you prepare a preview.'}
            </p>
          </div>
        </aside>
      </div>
    </>
  )
}
function Sources({ sources, onClose }: { sources: CatalogSource[]; onClose: () => void }) {
  return (
    <Modal title="Catalogue sources & credits" onClose={onClose} wide>
      <p>
        Source records are bundled at the revisions shown here. Copying a recipe creates an
        independent library entry; it keeps the source attribution.
      </p>
      {sources.map((source) => (
        <section className="catalog-source" key={source.id}>
          <h3>{source.name}</h3>
          <p>{source.notes}</p>
          <dl>
            <div>
              <dt>Catalogue entries</dt>
              <dd>{source.entry_count.toLocaleString()}</dd>
            </div>
            <div>
              <dt>Original records</dt>
              <dd>{source.record_count.toLocaleString()}</dd>
            </div>
            <div>
              <dt>Excluded records</dt>
              <dd>{source.excluded_count.toLocaleString()}</dd>
            </div>
            <div>
              <dt>Attribution</dt>
              <dd>{source.attribution}</dd>
            </div>
            <div>
              <dt>License</dt>
              <dd>{source.license}</dd>
            </div>
          </dl>
          <small>
            Revision <code>{source.commit}</code>
          </small>
          {externalURL(source.repository) && (
            <p>
              <a href={externalURL(source.repository)} target="_blank" rel="noreferrer">
                Open source repository
              </a>
            </p>
          )}
        </section>
      ))}
    </Modal>
  )
}
function References({ model, onClose }: { model: string; onClose: () => void }) {
  const resource = useResource<CatalogReferences>(
    `/catalog/references?model=${encodeURIComponent(model)}`
  )
  const images =
    resource.data?.assets.filter(
      (asset) =>
        asset.bundled &&
        asset.content_type.startsWith('image/') &&
        (asset.kind.includes('machine') || asset.kind.includes('device'))
    ) || []
  return (
    <Modal title="Machine guides & artwork" onClose={onClose} wide>
      <p>
        Bundled and installed guides open from this Hub. Guides marked “External archive” open their
        archived source.
      </p>
      <ErrorBanner message={resource.error} retry={resource.reload} />
      {resource.loading ? (
        <Loading />
      ) : (
        <>
          <div className="machine-artwork">
            {images.map((asset) => (
              <figure key={asset.path}>
                <img src={catalogAssetURL(asset.url)} alt={asset.title} />
                <figcaption>{asset.title}</figcaption>
              </figure>
            ))}
          </div>
          <div className="reference-list">
            {resource.data?.items.map((item) => {
              const localURL = item.available ? catalogAssetURL(item.url) : undefined
              const url = localURL || externalURL(item.download_url || item.source_url)
              return (
                <article key={item.id}>
                  <div>
                    <strong>{item.title}</strong>
                    <p>{item.description}</p>
                    <small>{item.models.map(displayModel).join(' · ')}</small>
                  </div>
                  {url ? (
                    <a href={url} target="_blank" rel="noreferrer">
                      {localURL ? 'Open guide' : 'External archive'}
                      <ArrowRight size={14} />
                    </a>
                  ) : (
                    <span className="muted">Unavailable</span>
                  )}
                </article>
              )
            })}
          </div>
        </>
      )}
    </Modal>
  )
}
