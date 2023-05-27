# Arrow

## Example
```elixir
# this demo will create an SQLite3 file, my_db.db
# create a table if not exists foo (col)
# insert a random number between 0 and 1000 to the table
iex> {:ok, _arrow_array_stream_bin, _rows_affected} = Arrow.adbc_example
{:ok,
 <<28, 98, 183, 18, 1, 0, 0, 0, 60, 91, 183, 18, 1, 0, 0, 0, 56, 80, 183, 18,
   1, 0, 0, 0, 60, 98, 183, 18, 1, 0, 0, 0, 0, 154, 0, 16, 1, 0, 0, 0>>, -1}
```

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `arrow` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:arrow, "~> 0.1.0"}
  ]
end
```

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at <https://hexdocs.pm/arrow>.

