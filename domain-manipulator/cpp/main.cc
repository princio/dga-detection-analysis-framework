
#include <arrow/api.h>
#include <arrow/compute/api_aggregate.h>
#include <arrow/compute/api_scalar.h>
#include <arrow/compute/type_fwd.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <arrow/status.h>

#include <iostream>
#include <vector>

arrow::Status CallRScalarUDF(arrow::compute::KernelContext* context,
                             const arrow::compute::ExecSpan& span,
                             arrow::compute::ExecResult* result) {
  if (result->is_array_span()) {
    return arrow::Status::NotImplemented("ArraySpan result from R scalar UDF");
  }

  return SafeCallIntoRVoid(
      [&]() {
        auto kernel =
            reinterpret_cast<const arrow::compute::ScalarKernel*>(context->kernel());
        auto state = std::dynamic_pointer_cast<RScalarUDFKernelState>(kernel->data);

        cpp11::writable::list args_sexp(span.num_values());

        for (int i = 0; i < span.num_values(); i++) {
          const arrow::compute::ExecValue& exec_val = span[i];
          if (exec_val.is_array()) {
            args_sexp[i] = cpp11::to_r6<arrow::Array>(exec_val.array.ToArray());
          } else if (exec_val.is_scalar()) {
            args_sexp[i] = cpp11::to_r6<arrow::Scalar>(exec_val.scalar->GetSharedPtr());
          }
        }

        cpp11::sexp batch_length_sexp = cpp11::as_sexp(span.length);

        std::shared_ptr<arrow::DataType> output_type = result->type()->GetSharedPtr();
        cpp11::sexp output_type_sexp = cpp11::to_r6<arrow::DataType>(output_type);
        cpp11::writable::list udf_context = {batch_length_sexp, output_type_sexp};
        udf_context.names() = {"batch_length", "output_type"};

        cpp11::sexp func_result_sexp =
            cpp11::function(state->exec_func_)(udf_context, args_sexp);

        if (Rf_inherits(func_result_sexp, "Array")) {
          auto array = cpp11::as_cpp<std::shared_ptr<arrow::Array>>(func_result_sexp);

          // Error for an Array result of the wrong type
          if (!result->type()->Equals(array->type())) {
            return cpp11::stop(
                "Expected return Array or Scalar with type '%s' from user-defined "
                "function but got Array with type '%s'",
                result->type()->ToString().c_str(), array->type()->ToString().c_str());
          }

          result->value = std::move(array->data());
        } else if (Rf_inherits(func_result_sexp, "Scalar")) {
          auto scalar = cpp11::as_cpp<std::shared_ptr<arrow::Scalar>>(func_result_sexp);

          // handle a Scalar result of the wrong type
          if (!result->type()->Equals(scalar->type)) {
            return cpp11::stop(
                "Expected return Array or Scalar with type '%s' from user-defined "
                "function but got Scalar with type '%s'",
                result->type()->ToString().c_str(), scalar->type->ToString().c_str());
          }

          auto array = ValueOrStop(
              arrow::MakeArrayFromScalar(*scalar, span.length, context->memory_pool()));
          result->value = std::move(array->data());
        } else {
          cpp11::stop("arrow_scalar_function must return an Array or Scalar");
        }
      },
      "execute scalar user-defined function");
}
     



arrow::Status RunMain(int argc, char const *argv[]) {
  arrow::io::IOContext io_context = arrow::io::default_io_context();

  std::shared_ptr<arrow::io::ReadableFile> file;
  auto result = arrow::io::ReadableFile::Open("tldlist.csv");
  if (result.ok()) {
    file = result.ValueOrDie();
  }

  std::shared_ptr<arrow::io::InputStream> input = file;

  auto read_options = arrow::csv::ReadOptions::Defaults();
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();

  // Instantiate TableReader from input stream and options
  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_options, parse_options, convert_options);
  if (!maybe_reader.ok()) {
    // Handle TableReader instantiation error...
  }
  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;

  // Read table from CSV file
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    // Handle CSV read error
    // (for example a CSV syntax error or failed type conversion)
  }
  std::shared_ptr<arrow::Table> table = *maybe_table;

  for (auto &&item : table->ColumnNames()) {
    std::cout << item << std::endl;
  }

  // std::cout << table->ToString();

  auto function_registry = arrow::compute::GetFunctionRegistry();

  std::cout << function_registry->num_functions();

  arrow::compute::Arity arity;
  arity.num_args = 0;

  arrow::compute::FunctionDoc doc;
  doc.arg_names = {};

  arrow::compute::SplitOptions options();

  auto function = arrow::compute::ScalarFunction("search_suffixes", arity, doc);

  arrow::compute::InputType input_type(arrow::StringScalar);

  std::vector<arrow::compute::InputType> in_types(0);

  arrow::StringType out_data_type();
  
  arrow::compute::OutputType out_type();

  arrow::compute::ScalarKernel kernel(in_types, out_type)

  // function.AddKernel()
  if (function_registry->CanAddFunction(function))

  return arrow::Status::OK();
}

int main(int argc, char const *argv[]) {
  arrow::Status status = RunMain(argc, argv);
  if (!status.ok()) {
    std::cerr << status.ToString() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "stocazzo";

  return 0;
}
