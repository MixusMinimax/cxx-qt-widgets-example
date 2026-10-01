<table><tr>
  <td>
    <img alt="Screenshot of a blood pressure graph, with the mouse cursor hovering over a data point" src="https://github.com/user-attachments/assets/8c713f79-3be8-486a-aaff-039e7099d829"/>
  </td>
  <td>
    <img alt="Screenshot of a modal with input fields for editing a measurement" src="https://github.com/user-attachments/assets/f7d7ce7f-94b3-4871-9245-6e537c4c1aa0"/>
  </td>
</tr></table>

This is a small demo utilizing the following technologies:

- QT6 Widgets
- [QCustomPlot](https://www.qcustomplot.com/)
- SQLite3
- [Diesel](https://diesel.rs/)
- [CXX](https://cxx.rs/)
- [CXX-QT](https://kdab.github.io/cxx-qt/book/)

### Features

This demo contains a single graph that allows the user to enter, edit,
delete, and view blood pressure measurements: systolic pressure, diastolic
pressure, mean arterial pressure, and pulse.

Pressure measurements can be read against the left y-axis in mmHg, while
pulse can be read against the right y-axis in 1/min.

Measurements are always kept in tuples of these values, it is not possible
to enter, for instance, only the diastolic pressure for a given data point.

#### Interaction

- Move the mouse over the graph to get a crosshair for accurate readout
- The y-axes show tags with the exact value of the current cursor position
- Moving the cursor close to a data point snaps the cursor to it and shows
  the measurement it belongs to, and highlights its specific entry under
  the cursor.
- If the cursor itself is not close to a value, but its vertical axis is
  close to a measurement, the measurement is shown too, with no specific
  entry highlighted.
- Double-clicking while a measurement is under the cursor opens a modal to
  allow editing it.
- Double-clicking anywhere else on the graph opens the modal to enter
  values for a new measurement. The date is pre-filled to the position of
  the cursor.
- If not entered, the mean arterial pressure (MAP) defaults
  to $\text{map} = 1/3 * \text{sys} + 2/3 * \text{dia}$.
- Scrolling the mouse wheel while the cursor is over the graph adjusts the
  date range.
- Dragging the cursor over the graph slides the date range in that
  direction.
- Pressing one of the three zoom presets adjusts the date range to either
  1. encompass the entire set of measurements
  2. the current date
  3. the current week
- The viewed date range is remembered between program starts.

#### Data

- Edits are automatically synchronized into a `*.db` file using SQLite. The
  path for that file can be edited in `Edit/Preferences`.
- Data can be exported in `File/Export` (C-e) into a csv file.[^1]
- Data can be imported in `File/Import` (C-i) from a csv file.[^2]
- Measurements are unique by timestamp.
- On import, measurements that collide with existing measurements will
  replace[^3] existing measurements. Other existing measurements are
  retained.
- Import and export file pickers separately remember the previously used
  directory.

For more features to come, read [TODO.md](./TODO.md).

[^1]: There are currently no options for export, like delimiter or headers
in the UI, but it is implemented in code.

[^2]: There are currently no options for import, like delimiter or headers
in the UI, but it is implemented in code. CSV headers must be present and
correct, but the case is irrelevant. In the future, after selecting the
file, it will be possible to select (and even auto-detect) the delimiter
and header fields.

[^3]: In code, there already exist two strategies: Keep existing
measurements, or replace them with the imported ones. Aforementioned Modal
will let you choose this.
