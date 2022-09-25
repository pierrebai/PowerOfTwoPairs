#include <chrono>
#include <string>
#include <vector>

// Where command-line arguments are stored.
// 
// Allows providing member-data pointers to tell the command-line parser
// where to store the data it parses.
struct command_line_data_t
{
   virtual void validate() = 0;
};

// Description of one command-line argument.
struct command_line_arg_t
{
   std::string name;
   std::string short_option;
   std::string long_option;
   size_t command_line_data_t::* count;
   int64_t command_line_data_t::* number;
   bool command_line_data_t::* flag;

   bool is_flag() const { return flag != nullptr; }
   bool is_count() const { return count != nullptr; }
   bool is_number() const { return number != nullptr; }
};

// Helper function to convert pointer-to-member of classes derived
// from command_line_data_t to point-to-member of command_line_data_t.
// We assume the correct corrersponding derived class to be filled
// will be used when parsing the command-line arguments.
template <class T, class D>
inline T command_line_data_t::* make_arg(T D::* memeber)
{
   return static_cast<T command_line_data_t::*>(memeber);
}


// Parser for command-line arguments.
// Receives the description of the expected arguments and where to store them.
void parse_command_line(command_line_data_t& destination, const std::vector<command_line_arg_t>& args, const int argc, const char** argv);

// Measure elapsed time in seconds.
struct duration_t
{
   duration_t();

   std::chrono::seconds elapsed() const;

private:
   const std::chrono::steady_clock::time_point start_time;
};

