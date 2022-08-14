#include <irods/rodsClient.h> // For load_client_api_plugins()
#include <irods/rcConnect.h>
#include <irods/specificQuery.h>

#include <fmt/format.h>

class fancy_integer
{
public:
    void set(int v)
    {
	    value = v;
    }

    [[nodiscard]] auto get() const noexcept -> int
    {
	    return value;
    }

 private:
    int value;
};

int func_with_many_params(int a,
                          int b,
                          int c,
                          int d,
                          int aa,
                          int bb,
                          int cc,
                          int dd)
{
	return 0;
}

int main()
{
	int long_variable_name_for_function_1 = 0;
	func_with_many_params(
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    long_variable_name_for_function_1,
	    func_with_many_params(long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1,
	                          long_variable_name_for_function_1));

	rodsEnv env;

	if (const int ec = getRodsEnv(&env); ec != 0) {
		// Failed to load the local environment information (i.e.
		// irods_environment.json). Handle error.
		return 1;
	}

	rErrMsg_t error;
	RcComm* conn = rcConnect(env.rodsHost,
	                         env.rodsPort,
	                         env.rodsUserName,
	                         env.rodsZone,
	                         0,
	                         &error);

	if (!conn) {
		// Failed to connect to server.
		// Handle error.
		return 1;
	}

	load_client_api_plugins();

	// We've successfully connected to an iRODS server.
	// Here's how you authenticate (i.e. log in) with the server.
	if (const int ec = clientLogin(conn); ec != 0) {
		// Failed to authenticate with server.
		// Handle error and disconnect.
		rcDisconnect(conn);
		return 1;
	}

	//
	// Use the "conn" pointer to execute operations.
	//

	SpecificQueryInp input{}; // Curly braces are equivalent to using
	                          // std::memset to clear the object.

	char query_alias[] = "ShowCollAcls";
	input.sql = query_alias;

	char input_arg[] = "/tempZone/home/kory";
	input.args[0] = input_arg;

	// Fetch the maximum number of rows for a single page.
	// This is specific to iRODS. The maximum page size has nothing to do with
	// the database.
	input.maxRows = MAX_SQL_ROWS;

	// So, we've set up our query. Now, we need to execute it. To do that,
	// "rcGenQuery" requires that we pass it a pointer. The pointer we give to
	// "rcGenQuery" will point to memory holding the results of our query.
	// Following execution of the query, if nothing goes wrong, we will be able
	// to use the pointer to iterate over the results. More on that later.
	GenQueryOut* output{};

	// We're all set. Execute the query and iterate/print the results!
	while (true) {
		if (const int ec = rcSpecificQuery(conn, &input, &output); ec != 0) {
			// Break out of the loop if we there aren't any results.
			if (ec == CAT_NO_ROWS_FOUND) {
				break;
			}

			// Failed to execute query.
			// Handle error.
			break;
		}

		//
		// At this point, we know "output" contains the information we're
		// interested in. All we have to do is print it.
		//

		// Iterate over each row.
		for (int row = 0; row < output->rowCnt; ++row) {
			fmt::print("row: ");

			// Iterate over each attribute (i.e. this is a column or the result
			// of an aggregate function).
			for (int attr = 0; attr < output->attriCnt; ++attr) {
				const SqlResult* sql_result = &output->sqlResult[attr];
				const char* value = sql_result->value + (row * sql_result->len);
				fmt::print("[{}] ", value);
			}

			fmt::print("\n");
		}

		// There's no more data, exit the loop.
		if (output->continueInx <= 0) {
			break;
		}

		// To move to the next page of the resultset, copy the continue index
		// contained in the output structure to the continue index of the input
		// structure. Forgetting to do this will result in an infinite loop!
		input.continueInx = output->continueInx;

		clearGenQueryOut(output);
	}

	clearGenQueryInp(&input);
	clearGenQueryOut(output);

	// When done, call rcDisconnect() so that the server knows the client is
	// finished. This lets the server know it is okay to deallocate resources.
	rcDisconnect(conn);

	return 0;
}
