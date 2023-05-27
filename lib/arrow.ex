defmodule Arrow do
  @moduledoc """
  Documentation for `Arrow`.
  """

  alias Adbc.Database
  alias Adbc.Connection
  alias Adbc.Statement

  @spec adbc_c_example :: {:ok, reference(), integer()} | {:error, String.t()}
  def adbc_c_example() do
    {:ok, database} = Database.new()
    :ok = Database.set_option(database, "driver", "adbc_driver_sqlite")
    :ok = Database.set_option(database, "uri", "file:my_db.db")
    :ok = Database.init(database)
    {:ok, connection} = Connection.new()
    :ok = Connection.init(connection, database)
    {:ok, statement} = Statement.new(connection)

    func_ptr = Adbc.get_function_pointer("AdbcStatementExecuteQuery")

    Statement.set_sql_query(statement, "CREATE TABLE IF NOT EXISTS foo (col)")
    :ok = Statement.prepare(statement)
    statement_ptr = Adbc.Statement.get_pointer(statement)

    {:ok, error} = Adbc.Error.new()

    {:ok, _array_stream, _rows_affected} =
      Arrow.Nif.arrow_execute_query_example(
        func_ptr,
        statement_ptr,
        error.pointer,
        {statement.reference, error.reference}
      )

    Statement.set_sql_query(statement, "INSERT INTO foo VALUES (#{:rand.uniform(1000)})")
    :ok = Statement.prepare(statement)
    statement_ptr = Adbc.Statement.get_pointer(statement)

    :ok = Adbc.Error.reset(error)
    {:ok, array_stream, rows_affected} =
      Arrow.Nif.arrow_execute_query_example(
        func_ptr,
        statement_ptr,
        error.pointer,
        {statement.reference, error.reference}
      )

    {array_stream, rows_affected}
  end

  @spec adbc_rust_example :: {:ok, reference(), integer()} | {:error, String.t()}
  def adbc_rust_example() do
    {:ok, database} = Database.new()
    :ok = Database.set_option(database, "driver", "adbc_driver_sqlite")
    :ok = Database.set_option(database, "uri", "file:my_db.db")
    :ok = Database.init(database)
    {:ok, connection} = Connection.new()
    :ok = Connection.init(connection, database)
    {:ok, statement} = Statement.new(connection)

    func_ptr = Adbc.get_function_pointer("AdbcStatementExecuteQuery")

    Statement.set_sql_query(statement, "CREATE TABLE IF NOT EXISTS foo (col)")
    :ok = Statement.prepare(statement)
    statement_ptr = Adbc.Statement.get_pointer(statement)

    {:ok, error} = Adbc.Error.new()

    {_array_stream, _rows_affected} =
      RustNIF.rust_adbc_statement_execute_query(
        func_ptr,
        statement_ptr,
        error.pointer,
        {statement.reference, error.reference}
      )

    Statement.set_sql_query(statement, "INSERT INTO foo VALUES (#{:rand.uniform(1000)})")
    :ok = Statement.prepare(statement)
    statement_ptr = Adbc.Statement.get_pointer(statement)

    :ok = Adbc.Error.reset(error)
    RustNIF.rust_adbc_statement_execute_query(
      func_ptr,
      statement_ptr,
      error.pointer,
      {statement.reference, error.reference}
    )
  end
end
