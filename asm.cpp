/*****************************************************************************
TITLE:      Source code for assembler (Language used : C++)

AUTHOR:     HARSH LOOMBA
ROLL NO:	2101CS32
Declaration of Authorship
This C++ file, asm.cpp, is part of the miniproject of CS209/CS210 at the
Department of Computer Science and Engg, IIT Patna.
*****************************************************************************/

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <algorithm>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>

using namespace std;

// Handles listing vector format
typedef struct listing
{
    int pc;
    int mem_code;
    int operand;
    bool op_backpatch = false;
    string code_line = "";
} Listings;

// mnemonics table
unordered_map<string, pair<int, int>> mnemonic = {
    {"data", {19, 1}},
    {"ldc", {0, 1}},
    {"adc", {1, 1}},
    {"ldl", {2, 2}},
    {"stl", {3, 2}},
    {"ldnl", {4, 2}},
    {"stnl", {5, 2}},
    {"add", {6, 0}},
    {"sub", {7, 0}},
    {"shl", {8, 0}},
    {"shr", {9, 0}},
    {"adj", {10, 1}},
    {"a2sp", {11, 0}},
    {"sp2a", {12, 0}},
    {"call", {13, 2}},
    {"return", {14, 0}},
    {"brz", {15, 2}},
    {"brlz", {16, 2}},
    {"br", {17, 2}},
    {"HALT", {18, 0}},
    {"SET", {20, 1}}};

// other required data structures

// relates mnemonic code to values taken as input
int code_val[21] = {1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 0, 0, 2, 0, 2, 2, 2, 0, 1, 1};

// symbol table
unordered_map<string, int> symbol_table;

// backpatch list
unordered_map<int, string> backpatch_list;

// data list
unordered_map<int, int> data_list;

// Used to store and display unused labels
set<string> unused_labels;

// Used to store errors, listing file lines respectively
vector<string> errors, list_str;

// Used to store warnings
vector<pair<int, string>> warnings;

// Store output integers
vector<unsigned int> output;

// Used to store the listing file data
vector<Listings> listings;

int PC = -1; // Program counter

// remove whitespace from right
string remove_right_whitespace(string s)
{
    int ind = s.length();
    string s0 = "";

    for (int i = s.length() - 1; i >= 0; i--)
    {
        if ((s[i] == ' ') || (s[i] == '\t'))
        {
            ind--;
        }
        else
        {
            break;
        }
    }

    for (int i = 0; i < ind; i++)
    {
        s0 += s[i];
    }

    return s0;
}

// remove whilespace from left
string remove_left_whitespace(string s)
{
    int ind = 0;
    string s0 = "";

    for (unsigned int i = 0; i < s.length(); i++)
    {
        if ((s[i] == ' ') || (s[i] == '\t'))
        {
            ind++;
        }
        else
        {
            break;
        }
    }

    for (unsigned int i = ind; i < s.length(); i++)
    {
        s0 += s[i];
    }

    return s0;
}

// remove whitespace from both sides
string remove_whitespace(string s)
{
    string s0 = remove_left_whitespace(s);
    s0 = remove_right_whitespace(s0);
    return s0;
}

// Check if it is possible to have the given label
bool is_possible_label(string s)
{
    // Should not me a mnemonic
    if (mnemonic.find(s) != mnemonic.end())
    {
        return false;
    }

    // Should not start with a number
    if ((s[0] >= int('0')) && (s[0] <= int('9')))
    {
        return false;
    }

    // Should be alphanumeric
    for (unsigned int i = 0; i < s.length(); i++)
    {
        if (!(((s[i] >= 'a') && (s[i] <= 'z')) || ((s[i] >= 'A') && (s[i] <= 'Z')) || ((s[i] >= '0') && (s[i] <= '9'))))
        {
            return false;
        }
    }

    return true;
}

// Check if the given string is a addable label
int is_valid_label(string s)
{
    // Should not be in symbol table (already declared)
    if (symbol_table.find(s) != symbol_table.end())
    {
        return 1;
    }

    // Should not be a mnemonic
    if (mnemonic.find(s) != mnemonic.end())
    {
        return 2;
    }

    // Should be alphanumeric
    for (unsigned int i = 0; i < s.length(); i++)
    {
        if (!(((s[i] >= 'a') && (s[i] <= 'z')) || ((s[i] >= 'A') && (s[i] <= 'Z')) || ((s[i] >= '0') && (s[i] <= '9'))))
        {
            return 3;
        }
    }

    // First letter is an alphabet
    if ((s[0] >= int('0')) && (s[0] <= int('9')))
    {
        return 4;
    }

    return 0;
}

// To check if the given string is a valid mnemonic
bool is_valid_mnemonic(string s)
{
    // Should exist in mnemonic table
    if (mnemonic.find(s) != mnemonic.end())
    {
        return 1;
    }
    return false;
}

// Used to check if the given string is a possible number
bool is_valid_number(string s)
{
    if ((s[0] == '0') && (s.length() >= 2))
    {
        if (s.length() >= 3)
        {
            // Binary
            if (s[1] == 'b')
            {
                for (unsigned int i = 2; i < s.length(); i++)
                {
                    if ((s[i] != '0') && (s[i] != '1'))
                    {
                        return 0;
                    }
                }

                return 2;
            }

            // Hexadecimal
            else if (s[1] == 'x')
            {
                for (unsigned int i = 2; i < s.length(); i++)
                {
                    if (!(((s[i] >= '0') && (s[i] <= '9')) || ((s[i] >= 'a') && (s[i] <= 'f')) || ((s[i] >= 'A') && (s[i] <= 'F'))))
                    {
                        return 0;
                    }
                }

                return 16;
            }
        }

        else
        {
            // Octal
            for (unsigned int i = 1; i < s.length(); i++)
            {
                if (!((s[i] >= '0') && (s[i] <= '7')))
                {
                    return 0;
                }
            }

            return 8;
        }
    }

    else if (((s[0] == '+') || (s[0] == '-')) && (s.length() >= 2))
    {
        // Decimal with sign
        for (unsigned int i = 1; i < s.length(); i++)
        {
            if (!((s[i] >= '0') && (s[i] <= '9')))
            {
                return 0;
            }
        }

        return 10;
    }

    else
    {
        // Decimal without sign
        for (unsigned int i = 0; i < s.length(); i++)
        {
            if (!((s[i] >= '0') && (s[i] <= '9')))
            {
                return 0;
            }
        }

        return 10;
    }

    return 0;
}

// Used to convert given number string to integer
int convert_to_num(string s)
{
    int num = 0;
    if ((s[0] == '0') && (s.length() >= 2))
    {
        if (s.length() >= 3)
        {
            // Binary
            if (s[1] == 'b')
            {
                s = s.substr(2, string::npos);
                num = bitset<24>(s).to_ullong();
            }

            // Hexadecimal
            else if (s[1] == 'x')
            {
                s = s.substr(2, string::npos);

                stringstream numstr;
                numstr << hex << s;

                numstr >> num;
            }
        }

        else
        {
            // Octal
            s = s.substr(1, string::npos);

            stringstream numstr;
            numstr << oct << s;

            numstr >> num;
        }
    }

    else
    {
        // Decimal
        stringstream numstr;
        numstr << s;

        numstr >> num;
    }

    return num;
}

// IMplement first pass
int first_pass(ifstream &asm_file)
{
    if (asm_file.is_open()) // If file open
    {
        string line;
        while (getline(asm_file, line))
        {
            PC++;

            // Remove comments
            line = line.substr(0, line.find(";", 0));

            // If line empty
            if (remove_whitespace(line) == "")
            {
                PC--;
                continue;
            }

            bool label_present = false;
            string label_name = "";

            // Checking for label
            size_t label_pos = line.find(":");

            if (label_pos != string::npos)
            {
                // Verifying if label is good, else adding error statemnts.

                label_present = true;

                label_name = line.substr(0, label_pos);
                label_name = remove_whitespace(label_name);

                switch (is_valid_label(label_name))
                {
                case 1:
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Duplicate label found.");
                    break;

                case 2:
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Label name should not be a reserved word.");
                    break;

                case 3:
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid label name, should be an alphanumberic string.");
                    break;

                case 4:
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid label name, should begin with a letter.");
                    break;
                }
            }

            // Used to store the line without label
            string line_wo_label;

            if (label_present)
            {
                if (label_pos + 2 < line.length())
                {
                    // Taking substring from ':' to end
                    line_wo_label = line.substr(label_pos + 1, string::npos);
                }
                else
                {
                    // Only label present
                    line_wo_label = "";
                }

                // Adding to symbol table
                symbol_table[label_name] = PC;

                // Adding to unused labels
                unused_labels.insert(label_name);

                // Adding to listings
                Listings lst;
                lst.code_line = label_name;
                lst.pc = PC;
                lst.mem_code = -1;
                lst.operand = 0;

                listings.push_back(lst);
            }
            else
            {
                // Otherwise line without label is the line itself
                line_wo_label = line;
            }

            if (remove_whitespace(line_wo_label) == "")
            {
                // Decrementing PC to preserve execution order. Since labels ae not present in final output code.
                PC--;
                continue;
            }

            stringstream ss(line_wo_label);

            string mnem;
            ss >> mnem;
            // Extracting mnemonic

            // Checking if mnemonic is there or not
            if (mnem == "")
            {
                continue;
            }

            if (!is_valid_mnemonic(mnem))
            {
                errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid mnemonic.");
                continue;
            }

            pair<int, int> op_val = mnemonic[mnem];

            if (op_val.first < 19)
            {
                // If no operand req
                if (op_val.second == 0)
                {
                    string nulstr;
                    ss >> nulstr;

                    if (nulstr != "")
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Unexpected operand.");
                        continue;
                    }

                    Listings lst;
                    lst.code_line = line;
                    lst.mem_code = op_val.first;
                    lst.operand = 0;
                    lst.pc = PC;

                    listings.push_back(lst);
                }

                // If value req.
                else if (op_val.second == 1)
                {
                    string valstr, nulstr;
                    ss >> valstr;
                    ss >> nulstr;

                    if (valstr == "")
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Missing operand.");
                        continue;
                    }

                    if (nulstr != "")
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Extra operand.");
                        continue;
                    }

                    Listings lst;
                    lst.code_line = line;
                    lst.mem_code = op_val.first;
                    lst.pc = PC;

                    // If number convert to number
                    if (is_valid_number(valstr))
                    {
                        lst.operand = convert_to_num(valstr);
                    }
                    else if (is_possible_label(valstr))
                    {
                        // If predefined label, replace with label name
                        if (symbol_table.find(valstr) != symbol_table.end())
                        {
                            lst.operand = symbol_table[valstr];

                            // If label represens data, replace with the data value
                            if (data_list.find(symbol_table[valstr]) != data_list.end())
                            {
                                lst.operand = data_list[symbol_table[valstr]];
                            }

                            // Removing from unused labels
                            if (unused_labels.find(valstr) != unused_labels.end())
                            {
                                unused_labels.erase(valstr);
                            }
                        }
                        else
                        {
                            // Else append to backpatch list
                            lst.operand = 0;
                            lst.op_backpatch = true;
                            backpatch_list[PC] = valstr;
                        }
                    }
                    else if ((valstr[0] >= '0') && (valstr[0] <= '9'))
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Not a number.");
                        continue;
                    }
                    else
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid operand.");
                        continue;
                    }

                    listings.push_back(lst);
                }

                // If label has offset as operand
                else
                {
                    string offsetstr, nulstr;
                    ss >> offsetstr;
                    ss >> nulstr;

                    if (offsetstr == "")
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Missing operand.");
                        continue;
                    }

                    if (nulstr != "")
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Extra operand.");
                        continue;
                    }

                    Listings lst;
                    lst.code_line = line;
                    lst.mem_code = op_val.first;
                    lst.pc = PC;

                    // If operand is number, set number as oprand
                    if (is_valid_number(offsetstr))
                    {
                        lst.operand = convert_to_num(offsetstr);
                    }

                    // If possible label check
                    else if (is_possible_label(offsetstr))
                    {
                        // If predefined label, set operand value
                        if (symbol_table.find(offsetstr) != symbol_table.end())
                        {

                            lst.operand = symbol_table[offsetstr] - PC - 1;

                            if (unused_labels.find(offsetstr) != unused_labels.end())
                            {
                                unused_labels.erase(offsetstr);
                            }
                        }
                        else
                        {
                            // Else append to backpatch list
                            lst.operand = 0;
                            lst.op_backpatch = true;
                            backpatch_list[PC] = offsetstr;
                        }
                    }

                    else if ((offsetstr[0] >= '0') && (offsetstr[0] <= '9'))
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Not a number.");
                        continue;
                    }
                    else
                    {
                        errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid operand.");
                        continue;
                    }

                    if ((lst.mem_code == 17) && (lst.operand == -1))
                    {
                        // Detecting infinite loops.
                        warnings.push_back({PC, "WARNING: Line " + to_string(PC) + ": Infinite loop detected."});
                    }

                    listings.push_back(lst);
                }
            }

            // Handling data
            else if (op_val.first == 19)
            {

                string valstr, nulstr;

                ss >> valstr;
                ss >> nulstr;

                if (valstr == "")
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Missing operand.");
                    continue;
                }

                if (nulstr != "")
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Extra operand.");
                    continue;
                }

                Listings lst;
                lst.code_line = line;
                lst.pc = PC;
                lst.mem_code = 19;

                // Convert given value to number, if its a valid number
                if (is_valid_number(valstr))
                {
                    lst.operand = convert_to_num(valstr);
                }
                else
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Not a number.");
                    continue;
                }

                // Add to data list
                data_list[PC] = lst.operand;

                listings.push_back(lst);
            }
            // Handling set
            else
            {
                if (!label_present)
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Invalid use of SET without a label.");
                    continue;
                }

                listings.pop_back();

                string valstr, nulstr;

                ss >> valstr;
                ss >> nulstr;

                if (valstr == "")
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Missing operand.");
                    continue;
                }

                if (nulstr != "")
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Extra operand.");
                    continue;
                }

                Listings lst;
                lst.code_line = line;
                lst.pc = PC;
                lst.mem_code = 20;

                if (is_valid_number(valstr))
                {
                    lst.operand = convert_to_num(valstr);
                }
                else
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": Not a number.");
                    continue;
                }

                symbol_table[label_name] = lst.operand;

                listings.push_back(lst);
            }
        }
    }
    // If file not open.
    else
    {
        errors.push_back("ERROR: File is empty.");
    }

    return 0;
}

int second_pass()
{
    for (auto it : listings)
    {
        // Convertin PC to hex
        stringstream spc;
        spc << setfill('0') << setw(8) << hex << it.pc;

        string hexPC(spc.str());

        // Except data, SET and labels
        if ((it.mem_code < 19) && (it.mem_code > -1))
        {
            if (it.op_backpatch)
            {
                // item is in backpatch list
                string op_label = backpatch_list[it.pc];

                // if in symbol table
                if (symbol_table.find(op_label) != symbol_table.end())
                {
                    // value
                    if (code_val[it.mem_code] == 1)
                    {
                        it.operand = symbol_table[op_label];

                        // If some data value exists for the label
                        if (data_list.find(symbol_table[op_label]) != data_list.end())
                        {
                            it.operand = data_list[symbol_table[op_label]];
                        }
                    }
                    // offset
                    else if (code_val[it.mem_code] == 2)
                    {
                        it.operand = symbol_table[op_label] - it.pc - 1;
                    }

                    // remove from unused labels
                    if (unused_labels.find(op_label) != unused_labels.end())
                    {
                        unused_labels.erase(op_label);
                    }
                }
                else
                {
                    errors.push_back("ERROR: Line " + to_string(PC) + ": No such label.");
                }
            }

            stringstream sop;

            // converiting operand to hex

            if (it.operand >= 0)
            {
                sop << setfill('0') << setw(6) << hex << it.operand;
            }
            else
            {
                stringstream sop0;
                sop0 << hex << it.operand;
                string hextrim;
                sop0 >> hextrim;
                hextrim = hextrim.substr(2, string::npos);
                sop << hextrim;
            }

            // converting mem code to hex

            sop << setfill('0') << setw(2) << hex << it.mem_code;

            string hexOP(sop.str());

            list_str.push_back(hexPC + " " + hexOP + " " + it.code_line);

            // combining and pushing the integer into output

            stringstream ssd;
            ssd << hex << hexOP;
            unsigned int bin_op;
            ssd >> bin_op;
            output.push_back(bin_op);
        }
        else if (it.mem_code == -1)
        {
            // if label, only add to listing file
            list_str.push_back(hexPC + "\t\t  " + it.code_line + ":");
        }
        else if (it.mem_code == 19)
        {

            // If data, add as 32 bit integer
            stringstream sop;
            sop << setfill('0') << setw(8) << hex << it.operand;
            string hexOP(sop.str());

            output.push_back(it.operand);
            list_str.push_back(hexPC + " " + hexOP + " " + it.code_line);
        }
        // if SET
        else
        {
            list_str.push_back(hexPC + "\t\t  " + it.code_line);
        }
    }

    for (auto it : unused_labels)
    {
        warnings.push_back({symbol_table[it], "WARNING : Line " + to_string(symbol_table[it]) + ": Unused label - " + it});
    }

    return 0;
}

int main(int argc, char **argv)
{
    // Checking input format
    if (!(argc == 2))
    {
        cout << "Invalid input format used. Valid Format is: ./asm asm_file.asm\n";
        return 0;
    }

    string asm_file_name = string(argv[1]);
    string file_name = asm_file_name.substr(0, asm_file_name.find(".", 0));

    ifstream asm_file;
    asm_file.open(asm_file_name, ios::in);

    // first pass
    first_pass(asm_file);

    asm_file.close();

    ofstream log_file, listing_file, object_file;

    string log_file_name = file_name + ".log"; //.log
    log_file.open(log_file_name, ios::out);

    // printing errors
    int err = 1;
    for (auto item : errors)
    {
        log_file << err << ". " << item << endl;
        err++;
    }

    if (err == 1)
    {
        log_file << "No Error in Pass 1." << endl;
    }

    if (errors.size() > 0)
    {
        // Empty files is errors in pass 1
        for (auto item : warnings)
        {
            log_file << err << ". " << item.second << endl;
            err++;
        }

        string listing_file_name = file_name + ".l";

        listing_file.open(listing_file_name, ios::out);
        listing_file.close();

        string object_file_name = file_name + ".o";

        object_file.open(object_file_name, ios::out | ios::binary);
        object_file.close();

        log_file.close();

        return 0;
    }

    // second pass
    second_pass();

    int err_cmp = err;

    // printing errors
    for (auto item : errors)
    {
        log_file << err << ". " << item << endl;
        err++;
    }

    if (err == err_cmp)
    {
        log_file << "No Error in Pass 2." << endl;
    }

    sort(warnings.begin(), warnings.end());

    // printing warnings
    for (auto item : warnings)
    {
        log_file << err << ". " << item.second << endl;
        err++;
    }

    if (errors.size() > 0)
    {
        // empty files if errors in pass 2
        string listing_file_name = file_name + ".l";

        listing_file.open(listing_file_name, ios::out);
        listing_file.close();

        string object_file_name = file_name + ".o";

        object_file.open(object_file_name, ios::out | ios::binary);
        object_file.close();

        log_file.close();

        return 0;
    }

    log_file.close();

    string listing_file_name = file_name + ".l";
    listing_file.open(listing_file_name, ios::out);

    string object_file_name = file_name + ".o";
    object_file.open(object_file_name, ios::out | ios::binary);

    // listing file
    for (auto it : list_str)
    {
        listing_file << it << endl;
    }

    // output file
    for (auto it : output)
    {
        object_file.write((const char *)&it, sizeof(int));
    }

    listing_file.close();
    object_file.close();

    return 0;
}