#include "helpers.hh"

// This assumes that a digit will be found and the line ends in " Kb".
int parse_stats_line(char* line){
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

// value is in KB
int virtualmem_usage() { 
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            result = parse_stats_line(line);
            break;
        }
    }
    fclose(file);
    return result;
}

void print_str_vector(vector<string> v) {
    for (auto i = v.begin(); i != v.end(); ++i) {
        cout << *i << endl;
    }
}

size_t nbits_for_int(int i) {
    assert(i >= 0);
    return floor(log(max(i, 1))/log(2)) + 1;
}

bool str_to_int(string s, int& i, int val_type_base) {
    try {
        size_t pos;
        i = stoi(s, &pos, val_type_base);
        // check whether the entire string was an integer
        return (s[pos] == '\0');
    } catch (const invalid_argument&) {
        i = -1;
        return false;
    }
}

void remove_char(string& str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
}

bool non_ascii(char c) {  
    return !isalnum(c);   
} 
void remove_non_ascii(string& str) { 
    str.erase(remove_if(str.begin(),str.end(), non_ascii), str.end());  
}

vector<string> split(string& str, char delim) {
    vector<string> data;
    stringstream ss(str);
    string substring;
    while (getline(ss, substring, delim)) {
        data.push_back(substring);
    }
    return data;
}

vector<string> split(string& str, string& delim) {
    vector<string> data;
    size_t pos = 0;
    string token;
    while ((pos = str.find(delim)) != std::string::npos) {
        token = str.substr(0, pos);
        data.push_back(token);
        str.erase(0, pos + delim.length());
    }    
    data.push_back(str);
    return data;
}

string read_file(string filename, string& str) {
    std::ifstream t(filename);
    t.seekg(0, ios::end);   
    str.reserve(t.tellg());
    t.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(t)),
                istreambuf_iterator<char>());
    return str;
}
