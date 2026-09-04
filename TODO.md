### view

- [X] Value tags on y-axes
- [X] denser grid
- [ ] measurement preview maybe?
    - [ ] potentially switch between axis tags and preview
- [ ] date range presets (today, this week, etc.)
    - [ ] default date range

### control

- [ ] clicking on existing measurement to edit
- [ ] click on graph, not on existing measurement, to add a new one

Will it be a modal? Like a pop-up? Or, will there be a separate view that
is a table, with editable rows.

For now, the former is easier, and will also teach me about subwindows. For
a table, I would have to create a QAbstractTableModel adapter, after going
through all the effort of having structured data...

### model

At first, simple in-memory model in rust using CXX.

It will later be an sqlite database using diesel-rs. This will teach me
about what I want to do with the git explorer. But also, it would allow you
to make custom queries: taking the average of every day when zooming out,
finding extreme values, tagging measurements in different ways, etc.
