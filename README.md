# Arrow

## Example
```elixir
iex> {:ok, r} = Arrow.int64_example
{:ok, #Reference<0.888448222.4180541443.5227>}   
iex> Arrow.to_arrow_c_data(r)
{:ok,
 <<246, 61, 104, 7, 1, 0, 0, 0, 251, 61, 104, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   ...>>,
 <<4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 3, 238, 56, 1, 0, 0, ...>>}
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

