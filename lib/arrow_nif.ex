defmodule Arrow.Nif do
  @moduledoc false

  @on_load :load_nif
  def load_nif do
    nif_file = ~c"#{:code.priv_dir(:arrow)}/arrow_nif"

    case :erlang.load_nif(nif_file, 0) do
      :ok -> :ok
      {:error, {:reload, _}} -> :ok
      {:error, reason} -> IO.puts("Failed to load nif: #{reason}")
    end
  end

  def arrow_int64_example(), do: :erlang.nif_error(:not_loaded)

  def arrow_utf8_example(), do: :erlang.nif_error(:not_loaded)

  def arrow_to_arrow_c_data(_data), do: :erlang.nif_error(:not_loaded)
end
