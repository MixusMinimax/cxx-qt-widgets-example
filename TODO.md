### view

- [X] Value tags on y-axes
- [ ] date tag on x-axis
- [X] denser grid
- [X] measurement preview maybe?
- [ ] date range presets (today, this week, etc.)
  - [ ] default date range
- [X] show errors sent back from backend in statusbar

### control

- [X] double-clicking on existing measurement to edit
- [X] double-clicking on graph, not on existing measurement, to add a new
  one
- [X] modal for editing / creating measurement
- [X] allow deleting measurement from modal
- [ ] check datetime in modal for collisions for better feedback
- [X] data export file picker
- [ ] data import file picker

### model

- [X] database migrations
- [X] rust data model
- [X] create
- [X] update
- [X] delete
- [X] query all
- [ ] query range
- [ ] query daily/weekly/monthly average
- [X] export csv
- [ ] import csv
  - if id is supplied, update those measurements
    - in case of TS collision, fail
    - in case of TS collision, warn (setting)
  - if id is not supplied, timestamp is identifying
    - in case of TS collision, replace existing measurement
    - in case of TS collision, warn and keep old (setting)
  - everything in one transaction
- [ ] undo/redo (audited database?)
