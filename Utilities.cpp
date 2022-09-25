#include "Utilities.h"

#include <exception>
#include <sstream>

using namespace std;

namespace
{
   void report_help(const char* program_name, const vector<command_line_arg_t>& args)
   {
      ostringstream ostr;
      ostr << program_name << " parameters:" << endl;
      for (const command_line_arg_t& param : args)
      {
         ostr
            << "   -" << param.short_option
            << " or --" << param.long_option
            << (param.is_flag() ? " [0 or 1]: " : " [value]: ")
            << param.name << endl;
      }
      throw runtime_error(ostr.str());
   }

   void report_parse_error(const char* program_name, const string& message)
   {
      ostringstream ostr;
      ostr << program_name << " error: " << message << endl;
      throw runtime_error(ostr.str());
   }

   void report_missing_argument(const char* program_name, const string& arg)
   {
      report_parse_error(program_name, string("Missing argument for ") + arg);
   }

   void report_unknown_parameter(const char* program_name, const string& arg)
   {
      report_parse_error(program_name, string("Unknown parameter given: ") + arg);
   }
}

void parse_command_line(command_line_data_t& destination, const vector<command_line_arg_t>& args, const int argc, const char** argv)
{
   const char* const program_name = argc >= 1 ? argv[0] : "Program";

   size_t current_implicit_param = 0;

   auto find_param = [&](const std::string& arg, int& arg_index) -> const command_line_arg_t*
   {
      if (arg.starts_with("--"))
      {
         if (arg == "--help")
            report_help(program_name, args);

         for (const command_line_arg_t& param : args)
         {
            if (arg.substr(2) == param.long_option)
            {
               arg_index += 1;
               return &param;
            }
         }
         return nullptr;
      }
      else if (arg.starts_with("-"))
      {
         if (arg == "-h")
            report_help(program_name, args);

         for (const command_line_arg_t& param : args)
         {
            if (arg.substr(1) == param.short_option)
            {
               arg_index += 1;
               return &param;
            }
         }
         return nullptr;
      }
      else
      {
         return nullptr;
      }
   };

   for (int arg_index = 1; arg_index < argc; ++arg_index)
   {
      string arg = argv[arg_index];
      const command_line_arg_t* to_parse = find_param(arg, arg_index);
      if (arg_index >= argc)
         report_missing_argument(program_name, arg);
      else if (to_parse == nullptr)
         report_unknown_parameter(program_name, arg);
      else if (to_parse->is_number())
         (destination.*to_parse->number) = int64_t(atol(argv[arg_index]));
      else if (to_parse->is_count())
         (destination.*to_parse->count) = size_t(atol(argv[arg_index]));
      else if (to_parse->is_flag())
         (destination.*to_parse->flag) = atol(argv[arg_index]) != 0;
   }

   destination.validate();
}

// Measure elapsed time in seconds.
duration_t::duration_t() : start_time(chrono::steady_clock::now()) {}

chrono::seconds duration_t::elapsed() const
{
   const auto end_time = chrono::steady_clock::now();
   return chrono::duration_cast<chrono::seconds>(end_time - start_time);
}
