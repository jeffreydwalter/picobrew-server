import { useEffect, useRef, useState, type FormEvent } from 'react'
import {
  Plus,
  Upload,
  Download,
  ArrowUpRight,
  ArrowDown,
  ArrowUp,
  Trash2,
  Send,
  Save,
  FlaskConical,
  Clock3,
  Wheat,
  ShoppingBasket,
  History,
  Copy,
  Sparkles,
  Check,
  Loader2,
  BookOpen,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, download, errorMessage } from '../lib/api'
import { dateLabel, ingredientUnits, newRecipe, slug } from '../lib/utils'
import { isNative, kindLabel, programTiming } from '../lib/programs'
import NativeRecipeEditor from './NativeRecipeEditor'
import { TransferDialog } from '../components/TransferDialog'
import type { Recipe, Ingredient, RecipeStep, Adapted } from '../types'
import {
  Badge,
  Button,
  Confirm,
  Empty,
  ErrorBanner,
  Field,
  IconButton,
  Loading,
  Modal,
  PageTitle,
  SearchBox,
  SectionHeading,
} from '../components/ui'
export default function Recipes({ id }: { id?: string }) {
  return id ? <RecipeEditor id={id} /> : <RecipeLibrary />
}
function RecipeLibrary() {
  const hub = useHub()
  const [query, setQuery] = useState('')
  const [importing, setImporting] = useState(false)
  const [selected, setSelected] = useState<string[]>([])
  const [shopping, setShopping] = useState(false)
  const [transfer, setTransfer] = useState(false)
  const shoppingIDs = selected.filter((id) =>
    hub.recipes.some((recipe) => recipe.id === id && !isNative(recipe))
  )
  const filtered = hub.recipes.filter((recipe) =>
    `${recipe.name} ${recipe.style} ${(recipe.tags || []).join(' ')}`
      .toLowerCase()
      .includes(query.toLowerCase())
  )
  return (
    <>
      <PageTitle
        eyebrow="A COLLECTION WORTH BREWING"
        title="Your recipe library."
        description="Create once. Refine with every brew. Adapt to the machine on your bench."
        actions={
          <>
            <Button variant="secondary" onClick={() => hub.navigate('/catalog')}>
              <BookOpen size={16} />
              Community catalogue
            </Button>
            <Button variant="secondary" onClick={() => setImporting(true)}>
              <Upload size={16} />
              Import recipe
            </Button>
            <Button onClick={() => hub.navigate('/recipes/new')}>
              <Plus size={16} />
              Create recipe
            </Button>
          </>
        }
      />
      <div className="list-toolbar">
        <SearchBox value={query} onChange={setQuery} placeholder="Search recipes, styles or tags" />
        <div className="inline-actions">
          <span className="muted">{filtered.length} recipes & programs</span>
          <Button variant="secondary" disabled={!selected.length} onClick={() => setTransfer(true)}>
            <Send size={16} />
            Send selected{selected.length ? ` (${selected.length})` : ''}
          </Button>
          <Button
            variant="secondary"
            disabled={!shoppingIDs.length}
            onClick={() => setShopping(true)}
          >
            <ShoppingBasket size={16} />
            Shopping list{shoppingIDs.length ? ` (${shoppingIDs.length})` : ''}
          </Button>
        </div>
      </div>
      {filtered.length ? (
        <div className="recipe-grid">
          {filtered.map((recipe, index) => (
            <article className="recipe-card" key={recipe.id}>
              <div className={`recipe-art tone-${index % 3}`}>
                <Wheat size={58} strokeWidth={1.1} />
                <div className="recipe-art-lines" />
                <label className="recipe-select">
                  <input
                    type="checkbox"
                    aria-label={`Select ${recipe.name}`}
                    checked={selected.includes(recipe.id)}
                    onChange={(event) =>
                      setSelected((values) =>
                        event.target.checked
                          ? [...values, recipe.id]
                          : values.filter((id) => id !== recipe.id)
                      )
                    }
                  />
                </label>
                <span>{recipe.style || kindLabel(recipe.kind)}</span>
              </div>
              <div className="recipe-card-content">
                <Badge tone={isNative(recipe) ? 'copper' : 'green'}>{kindLabel(recipe.kind)}</Badge>
                <h3>
                  <button onClick={() => hub.navigate(`/recipes/${recipe.id}`)}>
                    {recipe.name}
                  </button>
                </h3>
                <p>{recipe.description || 'A recipe ready for your next brew day.'}</p>
                <div className="recipe-meta">
                  {isNative(recipe) ? (
                    <>
                      <span>{recipe.source_model}</span>
                      <span>
                        <Clock3 size={14} />
                        {recipe.native_program
                          ? programTiming(recipe.native_program)
                          : 'Schedule unavailable'}
                      </span>
                      <span>Program only</span>
                    </>
                  ) : (
                    <>
                      <span>
                        <FlaskConical size={14} />
                        {recipe.batch_liters} L
                      </span>
                      <span>
                        <Clock3 size={14} />
                        {(recipe.steps || []).reduce(
                          (total, step) => total + step.minutes + step.drain_minutes,
                          0
                        )}{' '}
                        min
                      </span>
                      <span>{(recipe.ingredients || []).length} ingredients</span>
                    </>
                  )}
                </div>
                <div className="recipe-tags">
                  {(recipe.tags || []).slice(0, 3).map((tag) => (
                    <span key={tag}>{tag}</span>
                  ))}
                </div>
                <footer>
                  <small>Revision {recipe.revision}</small>
                  <button onClick={() => hub.navigate(`/recipes/${recipe.id}`)}>
                    Open recipe
                    <ArrowUpRight size={16} />
                  </button>
                </footer>
              </div>
            </article>
          ))}
        </div>
      ) : (
        <div className="panel">
          <Empty
            icon={<Wheat size={28} />}
            title={query ? 'No recipes match that search.' : 'Great brews start with an idea.'}
            description={
              query
                ? 'Try a different name or style.'
                : 'Create your first recipe, import BeerXML, or ask the brew assistant for a starting point.'
            }
            action={
              !query && (
                <Button onClick={() => hub.navigate('/recipes/new')}>
                  Create a recipe
                  <Plus size={16} />
                </Button>
              )
            }
          />
        </div>
      )}
      {importing && <ImportDialog onClose={() => setImporting(false)} />}{' '}
      {shopping && <ShoppingList ids={shoppingIDs} onClose={() => setShopping(false)} />}
      {transfer && (
        <TransferDialog
          recipes={hub.recipes}
          initialSelection={selected}
          onClose={() => setTransfer(false)}
        />
      )}
    </>
  )
}
function RecipeEditor({ id }: { id: string }) {
  const hub = useHub()
  const isNew = id === 'new'
  const loaded = useResource<Recipe>(isNew ? null : `/recipes/${id}`)
  const [recipe, setRecipe] = useState<Recipe>(() =>
    isNew && hub.draft ? { ...hub.draft, id: '', revision: 0 } : newRecipe()
  )
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const [dirty, setDirty] = useState(isNew)
  const [tagsInput, setTagsInput] = useState(isNew ? (hub.draft?.tags || []).join(', ') : '')
  const [deploy, setDeploy] = useState(false)
  const [remove, setRemove] = useState(false)
  const [revisions, setRevisions] = useState(false)
  const [advanced, setAdvanced] = useState('')
  const [advancedOpen, setAdvancedOpen] = useState(false)
  const originalRevision = useRef(0)
  useEffect(() => {
    if (loaded.data) {
      setRecipe({
        ...loaded.data,
        ingredients: loaded.data.ingredients || [],
        steps: loaded.data.steps || [],
        tags: loaded.data.tags || [],
      })
      originalRevision.current = loaded.data.revision
      setTagsInput((loaded.data.tags || []).join(', '))
      setDirty(false)
    }
  }, [loaded.data])
  useEffect(() => {
    if (isNew) hub.setDraft(null)
  }, [])
  useEffect(() => {
    const listener = (event: BeforeUnloadEvent) => {
      if (dirty) {
        event.preventDefault()
        event.returnValue = ''
      }
    }
    window.addEventListener('beforeunload', listener)
    return () => window.removeEventListener('beforeunload', listener)
  }, [dirty])
  function update(patch: Partial<Recipe>) {
    setRecipe((value) => ({ ...value, ...patch }))
    setDirty(true)
  }
  function updateIngredient(index: number, patch: Partial<Ingredient>) {
    update({
      ingredients: recipe.ingredients.map((item, i) =>
        i === index ? { ...item, ...patch } : item
      ),
    })
  }
  function updateStep(index: number, patch: Partial<RecipeStep>) {
    update({ steps: recipe.steps.map((item, i) => (i === index ? { ...item, ...patch } : item)) })
  }
  async function save(event?: FormEvent) {
    event?.preventDefault()
    setBusy(true)
    setError('')
    try {
      let payload = {
        ...recipe,
        tags: tagsInput
          .split(',')
          .map((tag) => tag.trim())
          .filter(Boolean),
      }
      if (advanced.trim()) {
        const extras = JSON.parse(advanced)
        if (!extras || Array.isArray(extras) || typeof extras !== 'object')
          throw new Error('Advanced metadata must be a JSON object.')
        payload = { ...payload, ...extras, id: recipe.id, revision: recipe.revision }
      }
      const saved = await api<Recipe>(isNew ? '/recipes' : `/recipes/${id}`, {
        method: isNew ? 'POST' : 'PUT',
        body: payload,
      })
      setRecipe(saved)
      originalRevision.current = saved.revision
      setDirty(false)
      await hub.refresh()
      hub.notify('Recipe saved to your library.')
      if (isNew) hub.navigate(`/recipes/${saved.id}`)
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  async function exportRecipe(format: string) {
    try {
      await download(
        `/recipes/${id}/export?format=${format}`,
        `${slug(recipe.name)}.${format === 'beerxml' ? 'xml' : 'json'}`
      )
    } catch (error) {
      hub.notify(errorMessage(error), 'error')
    }
  }
  function toggleAdvanced() {
    if (!advancedOpen && !advanced) {
      const {
        id,
        name,
        style,
        description,
        batch_liters,
        efficiency,
        ingredients,
        steps,
        tags,
        notes,
        revision,
        created_at,
        updated_at,
        type,
        boil_liters,
        boil_minutes,
        ...extras
      } = recipe as Recipe & Record<string, unknown>
      setAdvanced(JSON.stringify(extras, null, 2))
    }
    setAdvancedOpen(!advancedOpen)
  }
  if (!isNew && loaded.loading) return <Loading label="Opening your recipe…" />
  if (!isNew && loaded.error) return <ErrorBanner message={loaded.error} retry={loaded.reload} />
  const nativeRecipe = isNew ? recipe : loaded.data
  if (nativeRecipe && isNative(nativeRecipe))
    return <NativeRecipeEditor initial={nativeRecipe} isNew={isNew} />
  return (
    <>
      <PageTitle
        back={() => {
          if (!dirty || window.confirm('Leave this recipe without saving your changes?'))
            hub.navigate('/recipes')
        }}
        eyebrow={
          isNew
            ? 'A FRESH START'
            : `RECIPE · REVISION ${recipe.revision}${dirty ? ' · UNSAVED CHANGES' : ''}`
        }
        title={isNew ? 'What will you brew next?' : recipe.name || 'Untitled recipe'}
        description="A shared recipe, ready to be shaped for your next brew day."
        actions={
          <>
            {!isNew && (
              <Button
                variant="secondary"
                disabled={dirty}
                onClick={() => setDeploy(true)}
                title={dirty ? 'Save your changes before adapting this recipe' : ''}
              >
                <Send size={16} />
                Adapt & send
              </Button>
            )}
            <Button disabled={busy} onClick={() => void save()}>
              {busy ? <Loader2 size={16} className="spin" /> : <Save size={16} />}Save recipe
            </Button>
          </>
        }
      />
      <ErrorBanner message={error} />
      <div className="editor-layout">
        <div className="editor-main">
          <section className="panel">
            <SectionHeading title="The idea" />
            <div className="form-grid">
              <Field label="Recipe name">
                <input
                  value={recipe.name}
                  onChange={(event) => update({ name: event.target.value })}
                  placeholder="Sunday afternoon pale ale"
                  maxLength={100}
                  required
                />
              </Field>
              <Field label="Style">
                <input
                  value={recipe.style}
                  onChange={(event) => update({ style: event.target.value })}
                  placeholder="American Pale Ale"
                />
              </Field>
            </div>
            <Field label="Description">
              <textarea
                rows={2}
                value={recipe.description}
                onChange={(event) => update({ description: event.target.value })}
                placeholder="The flavors and character you're aiming for…"
              />
            </Field>
            <div className="form-grid thirds">
              <Field label="Batch size (L)">
                <input
                  type="number"
                  min="0.1"
                  step="0.1"
                  value={recipe.batch_liters}
                  onChange={(event) => update({ batch_liters: Number(event.target.value) })}
                />
              </Field>
              <Field label="Brewhouse efficiency (%)">
                <input
                  type="number"
                  min="1"
                  max="100"
                  value={recipe.efficiency}
                  onChange={(event) => update({ efficiency: Number(event.target.value) })}
                />
              </Field>
              <Field label="Tags">
                <input
                  value={tagsInput}
                  onChange={(event) => {
                    setTagsInput(event.target.value)
                    setDirty(true)
                  }}
                  placeholder="citrus, weekend"
                />
              </Field>
            </div>
          </section>
          <section className="panel">
            <SectionHeading
              title="Ingredients"
              description="Keep quantities in canonical units: malt in kg, hops in g and water in L. Adaptation scales the selected batch."
              actions={
                <Button
                  variant="secondary"
                  onClick={() =>
                    update({
                      ingredients: [
                        ...recipe.ingredients,
                        {
                          name: '',
                          kind: 'fermentable',
                          amount: 1,
                          unit: 'kg',
                          use: 'mash',
                          minutes: 0,
                        },
                      ],
                    })
                  }
                >
                  <Plus size={15} />
                  Add ingredient
                </Button>
              }
            />
            {recipe.ingredients.length ? (
              <div className="table-scroll">
                <table className="editor-table">
                  <thead>
                    <tr>
                      <th>Ingredient</th>
                      <th>Kind</th>
                      <th>Amount</th>
                      <th>Unit</th>
                      <th>Use</th>
                      <th>Minutes</th>
                      <th>Alpha %</th>
                      <th />
                    </tr>
                  </thead>
                  <tbody>
                    {recipe.ingredients.map((item, index) => (
                      <tr key={index}>
                        <td>
                          <input
                            aria-label={`Ingredient ${index + 1} name`}
                            value={item.name}
                            onChange={(event) =>
                              updateIngredient(index, { name: event.target.value })
                            }
                          />
                        </td>
                        <td>
                          <select
                            aria-label={`Ingredient ${index + 1} kind`}
                            value={item.kind}
                            onChange={(event) =>
                              updateIngredient(index, {
                                kind: event.target.value,
                                unit: ingredientUnits(event.target.value)[0],
                              })
                            }
                          >
                            <option value="fermentable">Fermentable</option>
                            <option value="hop">Hop</option>
                            <option value="yeast">Yeast</option>
                            <option value="water">Water</option>
                            <option value="misc">Other</option>
                          </select>
                        </td>
                        <td>
                          <input
                            aria-label={`Ingredient ${index + 1} amount`}
                            type="number"
                            min="0"
                            step="0.01"
                            value={item.amount}
                            onChange={(event) =>
                              updateIngredient(index, { amount: Number(event.target.value) })
                            }
                          />
                        </td>
                        <td>
                          <select
                            aria-label={`Ingredient ${index + 1} unit`}
                            value={item.unit}
                            onChange={(event) =>
                              updateIngredient(index, { unit: event.target.value })
                            }
                          >
                            {ingredientUnits(item.kind).map((unit) => (
                              <option key={unit}>{unit}</option>
                            ))}
                          </select>
                        </td>
                        <td>
                          <input
                            aria-label={`Ingredient ${index + 1} use`}
                            value={item.use}
                            onChange={(event) =>
                              updateIngredient(index, { use: event.target.value })
                            }
                          />
                        </td>
                        <td>
                          <input
                            aria-label={`Ingredient ${index + 1} minutes`}
                            type="number"
                            min="0"
                            value={item.minutes}
                            onChange={(event) =>
                              updateIngredient(index, { minutes: Number(event.target.value) })
                            }
                          />
                        </td>
                        <td>
                          <input
                            aria-label={`Ingredient ${index + 1} alpha acid`}
                            type="number"
                            min="0"
                            max="100"
                            step="0.1"
                            value={item.alpha || 0}
                            disabled={item.kind !== 'hop'}
                            onChange={(event) =>
                              updateIngredient(index, { alpha: Number(event.target.value) })
                            }
                          />
                        </td>
                        <td>
                          <IconButton
                            label={`Remove ingredient ${index + 1}`}
                            onClick={() =>
                              update({
                                ingredients: recipe.ingredients.filter((_, i) => i !== index),
                              })
                            }
                          >
                            <Trash2 size={15} />
                          </IconButton>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            ) : (
              <Empty
                icon={<Wheat size={24} />}
                title="Build the grain bill."
                description="Add fermentables, hops, yeast and anything else that makes this recipe yours."
              />
            )}
          </section>
          <section className="panel">
            <SectionHeading
              title="The brew schedule"
              description="Targets use Celsius in the shared recipe. Machine adaptation converts them as required."
              actions={
                <Button
                  variant="secondary"
                  onClick={() =>
                    update({
                      steps: [
                        ...recipe.steps,
                        {
                          name: 'New step',
                          kind: 'hold',
                          target_c: 67,
                          minutes: 10,
                          drain_minutes: 0,
                          location: 0,
                        },
                      ],
                    })
                  }
                >
                  <Plus size={15} />
                  Add step
                </Button>
              }
            />
            <div className="recipe-steps">
              {recipe.steps.map((step, index) => (
                <div className="recipe-step" key={index}>
                  <span className="step-number">{String(index + 1).padStart(2, '0')}</span>
                  <div className="step-fields">
                    <div className="form-grid">
                      <Field label="Step name">
                        <input
                          value={step.name}
                          onChange={(event) => updateStep(index, { name: event.target.value })}
                        />
                      </Field>
                      <Field label="Type">
                        <select
                          value={step.kind}
                          onChange={(event) =>
                            updateStep(index, {
                              kind: event.target.value,
                              ...(event.target.value === 'unheated' ? { target_c: 0 } : {}),
                            })
                          }
                        >
                          {[
                            'mash',
                            'mash-out',
                            'boil',
                            'hop',
                            'heat',
                            'hold',
                            'rest',
                            'unheated',
                            'cool',
                            'fermentation',
                          ].map((kind) => (
                            <option key={kind}>{kind}</option>
                          ))}
                        </select>
                      </Field>
                    </div>
                    <div className="form-grid quarters">
                      <Field label="Target (°C)">
                        <input
                          type="number"
                          min="0"
                          max="100"
                          step="0.1"
                          value={step.target_c}
                          onChange={(event) =>
                            updateStep(index, { target_c: Number(event.target.value) })
                          }
                        />
                      </Field>
                      <Field
                        label="Duration (min)"
                        hint={
                          step.kind === 'heat' || step.kind === 'cool'
                            ? '0 = reach the target temperature'
                            : undefined
                        }
                      >
                        <input
                          type="number"
                          min={step.kind === 'heat' || step.kind === 'cool' ? '0' : '1'}
                          max="43200"
                          value={step.minutes}
                          onChange={(event) =>
                            updateStep(index, { minutes: Number(event.target.value) })
                          }
                        />
                      </Field>
                      <Field label="Drain (min)">
                        <input
                          type="number"
                          min="0"
                          value={step.drain_minutes}
                          onChange={(event) =>
                            updateStep(index, { drain_minutes: Number(event.target.value) })
                          }
                        />
                      </Field>
                      <Field label="Compartment (0 = unmapped)">
                        <input
                          type="number"
                          min="0"
                          max="6"
                          value={step.location}
                          onChange={(event) =>
                            updateStep(index, { location: Number(event.target.value) })
                          }
                        />
                      </Field>
                    </div>
                  </div>
                  <div className="step-actions">
                    <IconButton
                      label={`Move step ${index + 1} up`}
                      disabled={!index}
                      onClick={() => {
                        const steps = [...recipe.steps]
                        ;[steps[index - 1], steps[index]] = [steps[index], steps[index - 1]]
                        update({ steps })
                      }}
                    >
                      <ArrowUp size={15} />
                    </IconButton>
                    <IconButton
                      label={`Move step ${index + 1} down`}
                      disabled={index === recipe.steps.length - 1}
                      onClick={() => {
                        const steps = [...recipe.steps]
                        ;[steps[index + 1], steps[index]] = [steps[index], steps[index + 1]]
                        update({ steps })
                      }}
                    >
                      <ArrowDown size={15} />
                    </IconButton>
                    <IconButton
                      label={`Remove step ${index + 1}`}
                      onClick={() => update({ steps: recipe.steps.filter((_, i) => i !== index) })}
                    >
                      <Trash2 size={15} />
                    </IconButton>
                  </div>
                </div>
              ))}
            </div>
          </section>
          <section className="panel">
            <SectionHeading
              title="Beyond the hot side"
              description="Recipe type, boil planning, fermentation and water chemistry travel with your recipe."
            />
            <div className="form-grid thirds">
              <Field label="Recipe type">
                <select
                  value={recipe.type || ''}
                  onChange={(event) => update({ type: event.target.value })}
                >
                  <option value="">Not specified</option>
                  <option>All Grain</option>
                  <option>Partial Mash</option>
                  <option>Extract</option>
                </select>
              </Field>
              <Field label="Boil volume (L)">
                <input
                  type="number"
                  min="0"
                  step="0.1"
                  value={recipe.boil_liters || ''}
                  onChange={(event) => update({ boil_liters: Number(event.target.value) })}
                />
              </Field>
              <Field label="Boil duration (min)">
                <input
                  type="number"
                  min="0"
                  value={recipe.boil_minutes || ''}
                  onChange={(event) => update({ boil_minutes: Number(event.target.value) })}
                />
              </Field>
            </div>
            <details className="ingredient-details">
              <summary>Ingredient specifications</summary>
              {recipe.ingredients.map((item, index) => (
                <div key={index}>
                  <h4>{item.name || `Ingredient ${index + 1}`}</h4>
                  <div className="form-grid quarters">
                    <Field label="Type">
                      <input
                        value={item.type || ''}
                        onChange={(event) => updateIngredient(index, { type: event.target.value })}
                        placeholder={
                          item.kind === 'fermentable'
                            ? 'Grain'
                            : item.kind === 'hop'
                              ? 'Bittering'
                              : 'Ale'
                        }
                      />
                    </Field>
                    <Field label="Form">
                      <input
                        value={item.form || ''}
                        onChange={(event) => updateIngredient(index, { form: event.target.value })}
                        placeholder={item.kind === 'hop' ? 'Pellet' : 'Dry'}
                      />
                    </Field>
                    <Field label="Yield (%)">
                      <input
                        type="number"
                        min="0"
                        max="100"
                        value={item.yield || ''}
                        onChange={(event) =>
                          updateIngredient(index, { yield: Number(event.target.value) })
                        }
                      />
                    </Field>
                    <Field label="Color (Lovibond)">
                      <input
                        type="number"
                        min="0"
                        value={item.color || ''}
                        onChange={(event) =>
                          updateIngredient(index, { color: Number(event.target.value) })
                        }
                      />
                    </Field>
                  </div>
                </div>
              ))}
            </details>
            <p className="help-text">
              Use advanced metadata below for water chemistry, fermentation stages and carbonation.
              These plans are preserved and exported; hot-side machine programs do not control them.
            </p>
          </section>
          <section className="panel">
            <SectionHeading title="Notes for brew day" />
            <Field label="Recipe notes">
              <textarea
                rows={4}
                value={recipe.notes}
                onChange={(event) => update({ notes: event.target.value })}
                placeholder="Water treatment, preparation, tasting goals…"
              />
            </Field>
            <button className="text-button" onClick={toggleAdvanced}>
              {advancedOpen ? 'Hide' : 'Show'} advanced BeerXML metadata
            </button>
            {advancedOpen && (
              <Field
                label="Water, fermentation, carbonation and other recipe metadata"
                hint="Existing extended fields are retained. Edit this JSON object to update recipe type, boil volume/time, water profile, fermentation stages or carbonation."
              >
                <textarea
                  className="code-input"
                  rows={12}
                  value={advanced}
                  onChange={(event) => {
                    setAdvanced(event.target.value)
                    setDirty(true)
                  }}
                />
              </Field>
            )}
          </section>
        </div>
        <aside className="editor-aside">
          <div className="recipe-summary">
            <p className="eyebrow">ON THE BREW SHEET</p>
            <Wheat size={38} strokeWidth={1.2} />
            <h3>{recipe.name || 'A recipe in the making'}</h3>
            <dl>
              <div>
                <dt>Batch</dt>
                <dd>{recipe.batch_liters} liters</dd>
              </div>
              <div>
                <dt>Ingredients</dt>
                <dd>{recipe.ingredients.length}</dd>
              </div>
              <div>
                <dt>Schedule</dt>
                <dd>
                  {recipe.steps.reduce(
                    (total, step) => total + step.minutes + step.drain_minutes,
                    0
                  )}{' '}
                  min
                </dd>
              </div>
              <div>
                <dt>Steps</dt>
                <dd>{recipe.steps.length}</dd>
              </div>
            </dl>
            <p>
              Save your recipe, then preview how it adapts to a specific machine before sending it.
            </p>
          </div>
          <div className="panel small-panel">
            <h3>Make it your own.</h3>
            <p>Need a starting point or another perspective?</p>
            <Button
              variant="secondary"
              className="full-width"
              onClick={() => hub.navigate('/assistant')}
            >
              <Sparkles size={15} />
              Ask the brew assistant
            </Button>
          </div>
          {!isNew && (
            <div className="editor-tools">
              <Button variant="ghost" onClick={() => void exportRecipe('json')}>
                <Download size={15} />
                Export JSON
              </Button>
              <Button variant="ghost" onClick={() => void exportRecipe('beerxml')}>
                <Download size={15} />
                Export BeerXML
              </Button>
              <Button variant="ghost" onClick={() => setRevisions(true)}>
                <History size={15} />
                Revision history
              </Button>
              <Button
                variant="ghost"
                onClick={() => {
                  hub.setDraft({ ...recipe, id: '', revision: 0, name: `${recipe.name} — copy` })
                  hub.navigate('/recipes/new')
                }}
              >
                <Copy size={15} />
                Duplicate recipe
              </Button>
              <Button variant="ghost" className="danger-text" onClick={() => setRemove(true)}>
                <Trash2 size={15} />
                Delete recipe
              </Button>
            </div>
          )}
        </aside>
      </div>
      {deploy && <AdaptDialog recipe={recipe} onClose={() => setDeploy(false)} />}{' '}
      {remove && (
        <Confirm
          title="Delete this recipe?"
          description="This removes it from the shared library. Existing brew history and recipes already on machines are retained."
          label="Delete recipe"
          danger
          busy={busy}
          onClose={() => setRemove(false)}
          onConfirm={async () => {
            setBusy(true)
            try {
              await api(`/recipes/${id}`, { method: 'DELETE' })
              await hub.refresh()
              hub.notify('Recipe removed.')
              hub.navigate('/recipes')
            } catch (error) {
              setError(errorMessage(error))
              setRemove(false)
            } finally {
              setBusy(false)
            }
          }}
        />
      )}{' '}
      {revisions && (
        <RevisionDialog
          id={id}
          onClose={() => setRevisions(false)}
          onLoad={(old) => {
            setRecipe({ ...old, id, revision: originalRevision.current })
            setTagsInput((old.tags || []).join(', '))
            setAdvanced('')
            setDirty(true)
            setRevisions(false)
            hub.notify(`Revision ${old.revision} loaded. Save to create a new revision.`)
          }}
        />
      )}
    </>
  )
}
function ImportDialog({ onClose }: { onClose: () => void }) {
  const hub = useHub()
  const [format, setFormat] = useState('beerxml')
  const [data, setData] = useState('')
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  async function submit() {
    setBusy(true)
    setError('')
    try {
      const items = await api<Recipe[]>('/recipes/import', {
        method: 'POST',
        body: { format, data },
      })
      await hub.refresh()
      hub.notify(`Imported ${items.length} ${items.length === 1 ? 'recipe' : 'recipes'}.`)
      onClose()
      if (items.length === 1) hub.navigate(`/recipes/${items[0].id}`)
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <Modal
      title="Bring your recipes along."
      onClose={onClose}
      wide
      actions={
        <>
          <Button variant="secondary" onClick={onClose}>
            Cancel
          </Button>
          <Button disabled={!data.trim() || busy} onClick={() => void submit()}>
            <Upload size={16} />
            {busy ? 'Importing…' : 'Import recipes'}
          </Button>
        </>
      }
    >
      <p>Import BeerXML or PicoBrew Hub JSON. Recipes are added as new library entries.</p>
      <div className="form-grid">
        <Field label="Format">
          <select value={format} onChange={(event) => setFormat(event.target.value)}>
            <option value="beerxml">BeerXML (.xml)</option>
            <option value="json">Recipe JSON (.json)</option>
          </select>
        </Field>
        <Field label="Choose a file">
          <input
            type="file"
            accept=".xml,.json,application/json,application/xml,text/xml"
            onChange={async (event) => {
              const file = event.target.files?.[0]
              if (file) {
                setFormat(file.name.toLowerCase().endsWith('.json') ? 'json' : 'beerxml')
                setData(await file.text())
              }
            }}
          />
        </Field>
      </div>
      <Field label="Or paste the recipe">
        <textarea
          className="code-input"
          rows={10}
          value={data}
          onChange={(event) => setData(event.target.value)}
          placeholder={format === 'beerxml' ? '<RECIPES>…</RECIPES>' : '{"name": "Your recipe", …}'}
        />
      </Field>
      <ErrorBanner message={error} />
    </Modal>
  )
}
function AdaptDialog({ recipe, onClose }: { recipe: Recipe; onClose: () => void }) {
  const hub = useHub()
  const first = hub.devices.find((device) => device.online)
  const [deviceID, setDeviceID] = useState(first?.id || '')
  const [profileID, setProfileID] = useState(first?.model || hub.profiles[0]?.id || '')
  const [batch, setBatch] = useState(recipe.batch_liters)
  const [adapted, setAdapted] = useState<Adapted | null>(null)
  const [busy, setBusy] = useState('')
  const [error, setError] = useState('')
  const target = hub.devices.find((device) => device.id === deviceID)
  const needsReady = target?.status?.busy || target?.status?.stopped || !target?.status?.ready
  async function preview() {
    setBusy('preview')
    setError('')
    try {
      setAdapted(
        await api<Adapted>(`/recipes/${recipe.id}/adapt`, {
          method: 'POST',
          body: { profile_id: profileID, batch_liters: batch },
        })
      )
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy('')
    }
  }
  async function deploy() {
    if (!adapted) return
    setBusy('deploy')
    try {
      await api(`/devices/${deviceID}/deploy`, {
        method: 'POST',
        body: {
          recipe_id: recipe.id,
          profile_id: profileID,
          batch_liters: batch,
          source_revision: adapted.source_revision,
        },
      })
      hub.notify(`Recipe sent to ${target?.name}. It has not been started.`)
      onClose()
      hub.navigate(`/devices/${deviceID}`)
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy('')
    }
  }
  return (
    <Modal
      title="Adapt the recipe. Choose its brewer."
      onClose={onClose}
      wide
      actions={
        <>
          <Button variant="secondary" onClick={onClose}>
            Cancel
          </Button>
          {adapted ? (
            <Button
              disabled={
                !adapted.can_deploy ||
                !target?.online ||
                !!needsReady ||
                target.model !== profileID ||
                !!busy
              }
              onClick={() => void deploy()}
            >
              <Send size={16} />
              {busy === 'deploy' ? 'Sending…' : 'Send to machine'}
            </Button>
          ) : (
            <Button disabled={!profileID || !!busy} onClick={() => void preview()}>
              {busy ? 'Adapting…' : 'Preview adaptation'}
              <ArrowUpRight size={16} />
            </Button>
          )}
        </>
      }
    >
      <p>
        Ingredient amounts are scaled for the selected batch and equipment profile. Sending a recipe
        never starts the brewer.
      </p>
      <div className="form-grid">
        <Field label="Target machine">
          <select
            value={deviceID}
            onChange={(event) => {
              const machine = hub.devices.find((device) => device.id === event.target.value)
              setDeviceID(event.target.value)
              if (machine) setProfileID(machine.model)
              setAdapted(null)
            }}
          >
            <option value="">Planning only</option>
            {hub.devices.map((device) => (
              <option key={device.id} value={device.id}>
                {device.name}
                {device.online ? '' : ' (offline)'}
              </option>
            ))}
          </select>
        </Field>
        <Field label="Equipment profile">
          <select
            value={profileID}
            onChange={(event) => {
              setProfileID(event.target.value)
              setAdapted(null)
            }}
          >
            {hub.profiles.map((profile) => (
              <option key={profile.id} value={profile.id}>
                {profile.name}
                {profile.control_supported ? '' : ' · planning & export'}
              </option>
            ))}
          </select>
        </Field>
        <Field label="Target batch size (L)">
          <input
            type="number"
            min="0.1"
            step="0.1"
            value={batch}
            onChange={(event) => {
              setBatch(Number(event.target.value))
              setAdapted(null)
            }}
          />
        </Field>
      </div>
      <ErrorBanner message={error} />
      {adapted && (
        <>
          <div className="adapt-summary">
            <Badge tone={adapted.can_deploy ? 'green' : 'amber'}>
              {adapted.can_deploy ? 'Machine recipe ready' : 'Planning profile'}
            </Badge>
            <strong>
              {adapted.recipe.batch_liters} L · {adapted.recipe.efficiency}% efficiency
            </strong>
          </div>
          {(adapted.warnings || []).map((warning, index) => (
            <p className="notice" key={index}>
              {warning}
            </p>
          ))}
          {needsReady && target && (
            <p className="notice">
              {target.name} must be idle and ready at its main menu before deployment.
            </p>
          )}
          <h3>Scaled ingredients</h3>
          <table className="data-table">
            <thead>
              <tr>
                <th>Ingredient</th>
                <th>Original</th>
                <th>Adapted</th>
              </tr>
            </thead>
            <tbody>
              {adapted.recipe.ingredients.map((item, index) => (
                <tr key={index}>
                  <td>{item.name}</td>
                  <td>
                    {recipe.ingredients[index]?.amount} {recipe.ingredients[index]?.unit}
                  </td>
                  <td>
                    <strong>
                      {Number(item.amount.toFixed(3))} {item.unit}
                    </strong>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
          <h3>Machine schedule</h3>
          {adapted.machine_recipe ? (
            <div className="table-scroll">
              <table className="data-table">
                <thead>
                  <tr>
                    <th>Step</th>
                    <th>Target</th>
                    <th>Time</th>
                    <th>Drain</th>
                    <th>Compartment</th>
                  </tr>
                </thead>
                <tbody>
                  {adapted.machine_recipe.steps.map((step, index) => (
                    <tr key={index}>
                      <td>{step.name}</td>
                      <td>{step.target_f}°F</td>
                      <td>{step.minutes} min</td>
                      <td>{step.drain_minutes} min</td>
                      <td>{step.location}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          ) : (
            <p>{adapted.profile.notes}</p>
          )}
          <button
            className="text-button"
            onClick={() => {
              hub.setDraft({
                ...adapted.recipe,
                id: '',
                revision: 0,
                name: `${recipe.name} · ${adapted.profile.name}`,
              })
              onClose()
              hub.navigate('/recipes/new')
            }}
          >
            Save this adaptation as a separate recipe
            <ArrowUpRight size={14} />
          </button>
        </>
      )}
    </Modal>
  )
}
function ShoppingList({ ids, onClose }: { ids: string[]; onClose: () => void }) {
  const resource = useResource<Ingredient[]>(
    `/shopping-list?recipes=${encodeURIComponent(ids.join(','))}`
  )
  return (
    <Modal title="Your brew-day shopping list" onClose={onClose}>
      <p>
        Combined ingredients for {ids.length} selected recipes. Different units remain separate.
      </p>
      {resource.loading ? (
        <Loading />
      ) : resource.error ? (
        <ErrorBanner message={resource.error} />
      ) : (
        <table className="data-table">
          <thead>
            <tr>
              <th>Ingredient</th>
              <th>Kind</th>
              <th>Amount</th>
            </tr>
          </thead>
          <tbody>
            {resource.data?.map((item, index) => (
              <tr key={index}>
                <td>{item.name}</td>
                <td>{item.kind}</td>
                <td>
                  {Number(item.amount.toFixed(3))} {item.unit}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </Modal>
  )
}
function RevisionDialog({
  id,
  onClose,
  onLoad,
}: {
  id: string
  onClose: () => void
  onLoad: (recipe: Recipe) => void
}) {
  const resource = useResource<Recipe[]>(`/recipes/${id}/revisions`)
  return (
    <Modal title="Recipe revision history" onClose={onClose}>
      <p>Load a prior version into the editor, then save it as a new revision.</p>
      {resource.loading ? (
        <Loading />
      ) : resource.error ? (
        <ErrorBanner message={resource.error} />
      ) : (
        <div className="revision-list">
          {resource.data?.map((recipe) => (
            <div key={recipe.revision}>
              <div>
                <strong>Revision {recipe.revision}</strong>
                <small>
                  {dateLabel(recipe.updated_at)} · {recipe.name}
                </small>
              </div>
              <Button variant="secondary" onClick={() => onLoad(recipe)}>
                Load revision
              </Button>
            </div>
          ))}
        </div>
      )}
    </Modal>
  )
}
