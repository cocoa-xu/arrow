defmodule Arrow do
  @moduledoc """
  Documentation for `Arrow`.
  """

  @spec int64_example :: {:ok, reference()} | {:error, String.t()}
  def int64_example do
    Arrow.Nif.arrow_int64_example()
  end

  @spec utf8_example :: {:ok, reference()} | {:error, String.t()}
  def utf8_example do
    Arrow.Nif.arrow_utf8_example()
  end

  @spec to_arrow_c_data(reference()) :: {:ok, binary(), binary()} | {:error, String.t()}
  def to_arrow_c_data(data) do
    Arrow.Nif.arrow_to_arrow_c_data(data)
  end
end
