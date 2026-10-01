### view

- [X] Value tags on y-axes
- [ ] date tag on x-axis
- [X] denser grid
- [X] measurement preview maybe?
- [X] date range presets (today, this week, etc.)
- [X] remember date range
- [X] show errors sent back from backend in statusbar
- [ ] shift-scroll for horizontal movement maybe?

### control

- [X] double-clicking on existing measurement to edit
- [X] double-clicking on graph, not on existing measurement, to add a new
  one
- [X] modal for editing / creating measurement
- [X] allow deleting measurement from modal
- [ ] check datetime in modal for collisions for better feedback
- [X] data export file picker
  - [ ] data export settings modal
  - [X] data export remember folder
- [X] data import file picker
  - [ ] data import settings modal
  - [X] data import remember folder

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
  - [ ] settings (delimiter, etc.)
- [X] import csv
  - [ ] settings (delimiter, etc.)
  - on id collisions, replace old row
  - on timestamp collisions, either:
    - keep old row
    - replace with new values
      - if id was not specified, keep old id
      - if id is new, delete old row
- [ ] undo/redo (audited database?)
