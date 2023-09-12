#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

// CONFIG OBJECT

/*[[clang::no_destroy]]*/ static uint64_t g_verbose = 0;
static std::uint32_t g_dummy = 1024 * 1024;
static
  int64_t g_fubar = 2048 * 2048;

using ConfigMap = std::map<std::string, std::vector<std::string>>;
/*[[clang::no_destroy]]*/ static std::map<std::string, std::vector<std::string>> g_config;

// STRING UTILITIES
static void trim(std::string& str)
{
    // ltrim
    auto pos = str.find_first_not_of(" \t");
    if (std::string::npos != pos) {
        str.erase(0, pos);
    }

    // rtrim
    pos = str.find_last_not_of(" \t");
    if (std::string::npos != pos) {
        str.erase(pos+1);
    }
}

class Source
{
public:
    Source() = default;

    bool process(const std::string& file)
    {
        std::ifstream ifs(file);
        if (!ifs.is_open()) {
            std::cerr << "FATAL: Unable to open file: " << file << "\n";
            return false;
        }

        // set up regex matches
        static const std::regex reInc(R"(^\s*#include <(\w+(\.h)?)>)");
        static const std::regex reStd(R"(\bstd::\w+\b)");
        // TODO: (std::)u?int\d\d?_t
        //static const std::regex reStd("str");
        //static const std::regex reInt(R"(\b[^:][^:]u?int\d\d?_t\b)");

        std::string line;
        while (getline(ifs, line)) {
            trim(line);

            allMatches(m_incList, line, reInc, 1);
            allMatches(m_refList, line, reStd, 0);
            //allMatches(g_stdList, line, reInt, 0);
        }

        /***
        // print all matches
        std::cout << "INCLUDES:\n";
        for (auto&& str : m_incList) {
            std::cout << str << "\n";
        }

        std::cout << "REFS:\n";
        for (auto&& str : m_refList) {
            std::cout << str << "\n";
        }
        ***/

        // build needed/unknown list
        for (auto&& ref : m_refList) {
            const auto it = g_config.find(ref);
            if (g_config.end() == it) {
                m_unknown.emplace(ref);
            }
            else {
                if (!contains(m_incList, it->second)) {
                    m_needed.emplace(it->second.at(0));
                }
            }
        }

        // show missing & unknown
        if (!m_unknown.empty()) {
            std::cout << "UNKNOWN:\n";
            for (auto&& nam : m_unknown) {
                std::cout << "- " << nam << "\n";
            }
        }

        if (!m_needed.empty()) {
            std::cout << "MISSING:\n";
            for (auto&& nam : m_needed) {
                std::cout << "- " << nam << "\n";
            }
        }

        // dummy statements for type scanning...
        if (m_refList.size() > g_dummy || m_refList.size() > static_cast<std::size_t>(g_fubar)) {
            std::cout << "!!! HUGE !!!\n";
        }

        return true;
    }


private:
    using StringSet = std::set<std::string>;

    template<typename T1, typename T2>
    bool contains(const T1& lst1, const T2& lst2)
    {
        for (auto&& xx : lst1) {
            for (auto&& yy : lst2) {
                if (xx == yy) {
                    return true;
                }
            }
        }

        return false;
    }

    std::size_t allMatches(StringSet& matchSet,
                           const std::string& str, 
                           const std::regex& re,
                           std::size_t idx)
    {
        // finding all the match.
        for (auto it = std::sregex_iterator(str.begin(), str.end(), re);
                it != std::sregex_iterator();
                it++) {
            std::smatch mat = *it;
            /***
              std::cout << "\nMatched string is: '" << match.str(0)
              << "', found at position: "
              << match.position(0) << std::endl;

              if (idx > 0) {
              std::cout << "Capture '" << match.str(idx)
              << "' at position " << match.position(idx) << std::endl;
              }
             ***/

            matchSet.emplace(mat.str(idx));
        }

        return matchSet.size();
    }

    StringSet m_incList;
    StringSet m_refList;
    StringSet m_needed;
    StringSet m_unknown;
    StringSet m_missing;
    StringSet m_extra;
};




static void usage(const std::string& arg0)
{
    const auto pos = arg0.rfind('/');
    const auto pgm = std::string::npos != pos ? arg0.substr(pos+1) : arg0;
    std::cout << "Usage: " << pgm << " [OPTS] <file(s)\n";
}


static std::vector<std::string> splitLine(const std::string& line, const std::string& sep)
{
    const auto sepLen = sep.length();
    std::vector<std::string> ret, blank;
    std::set<std::string> uniq;
    std::string::size_type pos(0), lpos(0);
    while (std::string::npos != (pos = line.find(sep, lpos))) {
        const auto len = pos - lpos;
        auto sub = line.substr(lpos, len);
        trim(sub);
        lpos = pos + sepLen;
 
        ret.emplace_back(sub);
        const auto rc = uniq.emplace(sub);
        if (!rc.second) {
            std::cerr << "FATAL: value: '" << sub << "' NOT unique in string: '" << line << "\n";
            return blank;
        }
    }
    
    auto sub = line.substr(lpos);
    trim(sub);
    ret.emplace_back(sub);
    const auto rc = uniq.emplace(sub);
    if (!rc.second) {
        std::cerr << "FATAL: value: '" << sub << "' NOT unique in string: '" << line << "\n";
        return blank;
    }

    return ret;
}

template<typename T>
static std::string joinLine(const T& vals, const std::string& sep)
{
    if (1 == vals.size()) {
        return *vals.begin();
    }

    std::ostringstream oss;
    bool first = true;
    for (auto&& val : vals) {
        if (!first) {
            oss << sep;
        }
        else {
            first = false;
        }
        oss << val;
    }
    return oss.str();
}

static void dumpConfig()
{
    for (auto&& rec : g_config) {
        std::cout << rec.first << " -> " << joinLine(rec.second, ", ") << "\n";
    }
}

static bool parseConfig(std::string& file)
{
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        std::cerr << "FATAL: Unable to open configuration file: " << file << "\n";
        return false;
    }

    /*[[clang::no_destroy]]*/ static const std::string sep = "=>";
    static const auto sepLen = sep.length();

    for (std::string line; getline(ifs, line); ) {
        if (line.empty() || '#' == line[0]) {
            continue;   // comment or empty
        }

        // split into 'what' and 'where'
        auto pos = line.find(sep);
        if (std::string::npos == pos) {
            std::cerr << "FATAL: No delimiter found in config line: '" << line << "'\n";
            return false;
        }
        std::string what = line.substr(0, pos);
        trim(what);

        std::string where = line.substr(pos + sepLen);
        trim(where);

        auto hdrs = splitLine(where, ",");
        if (hdrs.empty()) {
            return false;   // something bad happened
        }
        const auto rc = g_config.emplace(what, hdrs);
        if (!rc.second) {
            std::cerr << "FATAL: Unable to insert config value for: '" << what << "'\n";
            std::cerr << "CONFIG:\n";
            dumpConfig();
            return false;
        }

    }

    // dump the config
    /*
    std::cout << "SUCCESS!\n";
    dumpConfig();
    */

    return true;
}


int main(int argc, char **argv)
{
    int opt;
    std::string cfgFile, errStr;
    while ((opt = getopt(argc, argv, ":c:vh")) != -1) {
        errStr.clear();
        switch (opt) {
          case 'c':
            cfgFile = optarg;
            break;

          case 'v':
            ++g_verbose;
            break;

          case 'h':
            usage(argv[0]);
            return 0;

          case ':':
            errStr = "Missing argument for:";
            break;

          case '?':
            errStr = "Unknown option:";
            break;

          default: {
            std::ostringstream oss;
            oss << "Unexpected value: '" << static_cast<char>(opt) << "'";
            errStr = oss.str();
          } break;
        }

        if (!errStr.empty()) {
            std::cerr << "FATAL: " << errStr;
            if (':' == *errStr.rbegin()) {
                std::cerr << " '-" << static_cast<char>(optopt) << "'";
            }
            std::cerr << std::endl;
            return 22;
        }
    }

    // should be some arguments after the switches for the files to check
    if (optind == argc) {
        std::cerr << "ERROR: No files specified!\n";
        return 22;
    }

    if (cfgFile.empty()) {
        std::cerr << "ERROR: configuration file not specfied\n";
        return 22;
    }

    if (!parseConfig(cfgFile)) {
        return 2;
    }

    // config file parsed; start processing the input files
    for (int argx = optind; argx < argc; ++argx) {
        const char *thisFile = argv[argx];
        if (argx != optind) {
            std::cout << "\n";
        }
        
        std::cout << "Processing: " << thisFile << ":\n";
        Source src;
        if (!src.process(thisFile)) {
            std::cerr << "FAILED processing: " << thisFile << "\n";
        }
    }

    return 0;
}

