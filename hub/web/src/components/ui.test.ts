// @vitest-environment jsdom
import { createElement } from 'react'
import { afterEach, describe, expect, it } from 'vitest'
import { cleanup, render, screen } from '@testing-library/react'
import { Field } from './ui'

afterEach(cleanup)

describe('Field labels and descriptions', () => {
  it('keeps select options and hints out of the label while preserving caller IDs', () => {
    render(
      createElement(
        'div',
        null,
        createElement('p', { id: 'existing-help' }, 'Existing import instructions.'),
        createElement(Field, {
          label: 'Format',
          hint: 'Choose the recipe format.',
          children: createElement(
            'select',
            { id: 'import-format', 'aria-describedby': 'existing-help' },
            createElement('option', { value: 'beerxml' }, 'BeerXML (.xml)'),
            createElement('option', { value: 'json' }, 'Recipe JSON (.json)')
          ),
        })
      )
    )
    const control = screen.getByRole('combobox', { name: 'Format' }) as HTMLSelectElement
    expect(control.id).toBe('import-format')
    // Exact label matching uses native label text, including all descendant text.
    expect(Array.from(control.labels || [], (label) => label.textContent)).toEqual(['Format'])
    expect(control.labels?.[0].control).toBe(control)
    const descriptions = control.getAttribute('aria-describedby')!.split(/\s+/)
    expect(descriptions).toContain('existing-help')
    expect(descriptions.map((id) => document.getElementById(id)?.textContent)).toEqual([
      'Existing import instructions.',
      'Choose the recipe format.',
    ])
  })

  it('connects generated IDs to the correct input and textarea labels', () => {
    render(
      createElement(
        'div',
        null,
        createElement(Field, { label: 'Recipe name', children: createElement('input') }),
        createElement(Field, { label: 'Recipe notes', children: createElement('textarea') })
      )
    )
    const name = screen.getByRole('textbox', { name: 'Recipe name' }) as HTMLInputElement
    const notes = screen.getByRole('textbox', { name: 'Recipe notes' }) as HTMLTextAreaElement
    expect(name.id).not.toBe('')
    expect(notes.id).not.toBe('')
    expect(name.id).not.toBe(notes.id)
    expect(name.labels?.[0].control).toBe(name)
    expect(notes.labels?.[0].control).toBe(notes)
  })
})
