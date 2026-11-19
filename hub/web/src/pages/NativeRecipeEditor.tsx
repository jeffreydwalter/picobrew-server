import { useEffect, useState } from 'react'
import { Copy, Download, History, Save, Send, Trash2 } from 'lucide-react'
import type { Recipe } from '../types'
import { useHub, useResource } from '../context'
import { api, download, errorMessage } from '../lib/api'
import { copyProgram, kindLabel, programTiming } from '../lib/programs'
import { dateLabel, slug } from '../lib/utils'
import {
  Badge,
  Button,
  Confirm,
  ErrorBanner,
  Field,
  Loading,
  Modal,
  PageTitle,
  SectionHeading,
} from '../components/ui'
import { ProgramFields, Provenance } from '../components/Program'
import { TransferDialog } from '../components/TransferDialog'

export default function NativeRecipeEditor({
  initial,
  isNew,
}: {
  initial: Recipe
  isNew: boolean
}) {
  const hub = useHub()
  const [recipe, setRecipe] = useState<Recipe>(() => ({
    ...initial,
    tags: initial.tags || [],
    native_program: initial.native_program
      ? copyProgram(initial.native_program)
      : { id: '', name: initial.name, steps: [] },
  }))
  const [dirty, setDirty] = useState(isNew)
  const [tags, setTags] = useState((initial.tags || []).join(', '))
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const [transfer, setTransfer] = useState(false)
  const [revisions, setRevisions] = useState(false)
  const [remove, setRemove] = useState(false)
  const program = recipe.native_program!
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
  async function save() {
    setBusy(true)
    setError('')
    try {
      const saved = await api<Recipe>(isNew ? '/recipes' : `/recipes/${recipe.id}`, {
        method: isNew ? 'POST' : 'PUT',
        body: {
          ...recipe,
          native_program: { ...program, name: recipe.name },
          tags: tags
            .split(',')
            .map((tag) => tag.trim())
            .filter(Boolean),
        },
      })
      setRecipe(saved)
      setDirty(false)
      await hub.refresh()
      hub.notify('Machine program saved to your library.')
      if (isNew) hub.navigate(`/recipes/${saved.id}`)
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <>
      <PageTitle
        back={() => {
          if (!dirty || window.confirm('Leave this program without saving your changes?'))
            hub.navigate('/recipes')
        }}
        eyebrow={`${kindLabel(recipe.kind).toUpperCase()} · ${isNew ? 'NEW COPY' : `REVISION ${recipe.revision}`}${dirty ? ' · UNSAVED CHANGES' : ''}`}
        title={recipe.name || 'Untitled program'}
        description="Edit this library copy, then preview a transfer to the brewer’s saved recipe catalogue."
        actions={
          <>
            {!isNew && (
              <Button
                variant="secondary"
                disabled={dirty || busy}
                onClick={() => setTransfer(true)}
              >
                <Send size={16} />
                Preview & send
              </Button>
            )}
            <Button disabled={busy} onClick={() => void save()}>
              <Save size={16} />
              {busy ? 'Saving…' : 'Save program'}
            </Button>
          </>
        }
      />
      <ErrorBanner message={error} />
      <div className="editor-layout">
        <div className="editor-main">
          <section className="panel">
            <SectionHeading title="Program details" />
            <div className="form-grid">
              <Field label="Program name">
                <input
                  value={recipe.name}
                  maxLength={100}
                  onChange={(event) =>
                    update({
                      name: event.target.value,
                      native_program: { ...program, name: event.target.value },
                    })
                  }
                />
              </Field>
              <Field label="Style">
                <input
                  value={recipe.style}
                  onChange={(event) => update({ style: event.target.value })}
                />
              </Field>
            </div>
            <Field label="Description">
              <textarea
                rows={2}
                value={recipe.description}
                onChange={(event) => update({ description: event.target.value })}
              />
            </Field>
            <div className="form-grid">
              <Field label="Program type">
                <select
                  value={recipe.kind}
                  onChange={(event) => update({ kind: event.target.value })}
                >
                  <option value="native-program">Machine program</option>
                  <option value="maintenance">Cleaning program</option>
                </select>
              </Field>
              <Field
                label="Source machine"
                hint="This preserves the compartment layout of the original program."
              >
                <input readOnly value={recipe.source_model || ''} />
              </Field>
            </div>
            <Field label="Tags">
              <input
                value={tags}
                onChange={(event) => {
                  setTags(event.target.value)
                  setDirty(true)
                }}
              />
            </Field>
            <p className="notice">
              This record contains a machine schedule. Ingredient quantities, batch volume and
              efficiency are unspecified.
            </p>
          </section>
          <section className="panel">
            <ProgramFields
              program={program}
              onChange={(value) => update({ native_program: value })}
            />
          </section>
          <section className="panel">
            <SectionHeading title="Preparation & notes" />
            <Field label="Program notes">
              <textarea
                rows={5}
                value={recipe.notes}
                onChange={(event) => update({ notes: event.target.value })}
              />
            </Field>
            <Provenance value={recipe.provenance} />
          </section>
        </div>
        <aside className="editor-aside">
          <div className="recipe-summary">
            <Badge tone="copper">{kindLabel(recipe.kind)}</Badge>
            <h3>{recipe.name}</h3>
            <dl>
              <div>
                <dt>Source model</dt>
                <dd>{recipe.source_model}</dd>
              </div>
              <div>
                <dt>Steps</dt>
                <dd>{program.steps.length}</dd>
              </div>
              <div>
                <dt>Schedule</dt>
                <dd>{programTiming(program)}</dd>
              </div>
            </dl>
            <p>
              Zero-minute heating steps retain their original meaning. The transfer preview checks
              this brewer’s firmware and available slots.
            </p>
          </div>
          {!isNew && (
            <div className="editor-tools">
              <Button
                variant="ghost"
                onClick={async () => {
                  try {
                    await download(
                      `/recipes/${recipe.id}/export?format=json`,
                      `${slug(recipe.name)}.json`
                    )
                  } catch (error) {
                    setError(errorMessage(error))
                  }
                }}
              >
                <Download size={15} />
                Export program JSON
              </Button>
              <p className="help-text">
                Machine-only programs use JSON. BeerXML export requires an ingredient recipe.
              </p>
              <Button variant="ghost" onClick={() => setRevisions(true)}>
                <History size={15} />
                Revision history
              </Button>
              <Button
                variant="ghost"
                onClick={() => {
                  const name = `${recipe.name} copy`
                  hub.setDraft({
                    ...recipe,
                    id: '',
                    revision: 0,
                    name,
                    native_program: { ...copyProgram(program), name },
                  })
                  hub.navigate('/recipes/new')
                }}
              >
                <Copy size={15} />
                Duplicate program
              </Button>
              <Button variant="ghost" className="danger-text" onClick={() => setRemove(true)}>
                <Trash2 size={15} />
                Delete program
              </Button>
            </div>
          )}
        </aside>
      </div>
      {transfer && (
        <TransferDialog
          recipes={[recipe]}
          initialSelection={[recipe.id]}
          onClose={() => setTransfer(false)}
        />
      )}
      {revisions && (
        <ProgramRevisions
          id={recipe.id}
          onClose={() => setRevisions(false)}
          onLoad={(old) => {
            setRecipe({ ...old, id: recipe.id, revision: recipe.revision })
            setTags((old.tags || []).join(', '))
            setDirty(true)
            setRevisions(false)
          }}
        />
      )}
      {remove && (
        <Confirm
          title="Delete this library program?"
          description="Saved copies on your machines and existing brew history remain available."
          label="Delete program"
          danger
          busy={busy}
          onClose={() => setRemove(false)}
          onConfirm={async () => {
            setBusy(true)
            try {
              await api(`/recipes/${recipe.id}`, { method: 'DELETE' })
              await hub.refresh()
              hub.navigate('/recipes')
            } catch (error) {
              setError(errorMessage(error))
              setRemove(false)
            } finally {
              setBusy(false)
            }
          }}
        />
      )}
    </>
  )
}
function ProgramRevisions({
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
    <Modal title="Program revision history" onClose={onClose}>
      <p>Load a previous version, then save it as a new revision.</p>
      <ErrorBanner message={resource.error} />
      {resource.loading ? (
        <Loading />
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
