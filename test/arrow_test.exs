defmodule ArrowTest do
  use ExUnit.Case
  doctest Arrow

  test "greets the world" do
    assert Arrow.hello() == :world
  end
end
