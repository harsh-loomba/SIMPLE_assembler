/*****************************************************************************
TITLE:      Source code for emulator (Language used : C++)

AUTHOR:	    HARSH LOOMBA
ROLL NO:	2101CS32
Declaration of Authorship
This C++ file, emu.cpp, is part of the miniproject of CS209/CS210 at the
Department of Computer Science and Engg, IIT Patna.
*****************************************************************************/

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
using namespace std;

// registers
int SP = 0, A = 0, B = 0, PC = 0;

// memory
unsigned int memory[16777216] = {0};

// execution funtion table
void execute_function(int mem_code, int operand)
{
    switch (mem_code)
    {
    case 0: // ldc
        B = A;
        A = operand;
        break;
    case 1: // adc
        A += operand;
        break;
    case 2: // ldl
        B = A;
        A = memory[SP + operand];
        break;
    case 3: // stl
        memory[SP + operand] = A;
        A = B;
        break;
    case 4: // ldnl
        A = memory[A + operand];
        break;
    case 5: // stnl
        memory[A + operand] = B;
        break;
    case 6: // add
        A += B;
        break;
    case 7: // sub
        A = B - A;
        break;
    case 8: // shl
        A = B << A;
        break;
    case 9: // shr
        A = B >> A;
        break;
    case 10: // adj
        SP += operand;
        break;
    case 11: // a2sp
        SP = A;
        A = B;
        break;
    case 12: // sp2a
        B = A;
        A = SP;
        break;
    case 13: // call
        B = A;
        A = PC;
        PC += operand;
        break;
    case 14: // return
        PC = A;
        A = B;
        break;
    case 15: // brz
        if (A == 0)
        {
            PC += operand;
        }
        break;
    case 16: // brlz
        if (A < 0)
        {
            PC += operand;
        }
        break;
    case 17: // br
        PC += operand;
        break;
    }
}

// error format
void print_err_format()
{
    cout << "Invalid format used.\n";
    cout << "Usage: emu [option] file_name.o\n";
    cout << "-trace\t:\tShow instruction trace.\n";
    cout << "-before\t:\tShow memory dump before execution.\n";
    cout << "-after\t:\tShow memory dump after execution.\n";
    cout << "-isa\t:\tDisplay ISA.";
}

// reading a 32 bit integer from .o file
unsigned int read_number(fstream &file)
{
    unsigned int number = 0;

    file.read((char *)&number, sizeof(number));

    return number;
}

// get number of lines in .o file
unsigned int get_size(fstream &file)
{
    file.seekg(0, ios::end);
    unsigned int file_size = file.tellg();
    file.seekg(0, ios::beg);
    return file_size / 4;
}

// Reading the .o file
unsigned int read_file(fstream &input_file)
{
    unsigned int lines = get_size(input_file);

    unsigned int op;
    for (unsigned int i = 0; i < lines; i++)
    {
        op = read_number(input_file);
        // Store data in memory
        memory[i] = op;
    }

    return lines;
}

// decimal to hex
string to_hex(unsigned int decimal)
{
    stringstream hex_stream;
    hex_stream << setfill('0') << setw(8) << hex << decimal;
    return hex_stream.str();
}

// printing the memory dump
void print_memory_dump(unsigned int line, bool is_before)
{
    if (is_before)
    {
        cout << "Memory dump before execution" << endl;
    }
    else
    {
        cout << "Memory dump after execution" << endl;
    }

    for (unsigned int i = 0; i < line; i++)
    {
        if (i % 4 == 0)
        {
            cout << "\n0x" << to_hex(i);
        }
        cout << "\t0x" << to_hex(memory[i]);
    }

    cout << endl;
    return;
}

// Printing memory dump to file
void print_memory_dump_to_file(ofstream &file, unsigned int line)
{
    for (unsigned int i = 0; i < line; i++)
    {
        if (i % 4 == 0)
        {
            file << "\n0x" << to_hex(i);
        }
        file << "\t0x" << to_hex(memory[i]);
    }

    file << endl;
    return;
}

// Printing isa
void print_isa()
{
    string mnemonic_opp[19][2] = {
        {"ldc", "value"},
        {"adc", "value"},
        {"ldl", "offset"},
        {"stl", "offset"},
        {"ldnl", "offset"},
        {"stnl", "offset"},
        {"add", ""},
        {"sub", ""},
        {"shl", ""},
        {"shr", ""},
        {"adj", "value"},
        {"a2sp", ""},
        {"sp2a", ""},
        {"call", "offset"},
        {"return", ""},
        {"brz", "offset"},
        {"brlz", "offset"},
        {"br", "offset"},
        {"HALT", ""}};

    cout << "Opcode\tMnemonic\tOperand\n";
    cout << "-------------------------------\n";

    for (int i = 0; i < 19; i++)
    {
        cout << i << "\t" << mnemonic_opp[i][0] << "\t\t" << mnemonic_opp[i][1] << endl;
    }

    cout << "-------------------------------\n";

    return;
}

// executing the function
int execute(bool trace)
{
    SP = (1 << 23) - 1; // setting up Stack pointer
    unsigned int mem_code = 0, operand_l = 0, instr_no = 0;
    int operand = 0;

    // For valid memcodes
    while (mem_code < 18)
    {
        // extract mem_code (last 8 bits)
        mem_code = memory[PC] % 256;

        if (mem_code > 18)
        {
            return 1;
        }

        // extract operand (first 24 bits)
        operand_l = memory[PC] >> 8;

        if (operand_l > 8388607)
        {
            operand = (int)operand_l - 16777216;
        }
        else
        {
            operand = (int)operand_l;
        }

        // Printing trace, if given
        if (trace)
        {
            cout << "PC = 0x" << to_hex(PC);
            cout << "\tSP = 0x" << to_hex(SP);
            cout << "\tA = 0x" << to_hex(A);
            cout << "\tB = 0x" << to_hex(B);
            cout << endl;
        }

        // Incrementing PC and executing functions to simulate program.
        execute_function(mem_code, operand);

        PC++;
        instr_no++;
    }

    cout << endl;
    cout << instr_no << " instructions executed." << endl;

    return 0;
}

int main(int argc, char **argv)
{
    // Validating arguments
    if (argc != 3)
    {
        print_err_format();
        return 1;
    }

    // Checking and setting attributes
    bool trace = false, before = false, after = false, isa = false;
    unsigned int lines;
    string argument = string(argv[1]);

    if (argument == "-trace")
    {
        trace = true;
    }
    else if (argument == "-before")
    {
        before = true;
    }
    else if (argument == "-after")
    {
        after = true;
    }
    else if (argument == "-isa")
    {
        isa = true;
    }
    else
    {
        print_err_format();
        return 1;
    }

    fstream input_file(string(argv[2]), ios::binary | ios::in);

    // Reading file
    if (input_file.is_open())
    {
        lines = read_file(input_file);
        input_file.close();
    }
    else
    {
        cout << "ERROR: Empty file.";
        return 1;
    }

    // before mem dump
    if (before)
    {
        print_memory_dump(lines, true);
    }
    // isa
    else if (isa)
    {
        print_isa();
    }

    // executing and trace
    int err = execute(trace);

    // errenous program
    if (err == 1)
    {
        cout << "ERROR : Incorrect mnemonic encountered.";
        return 1;
    }

    // after mem dump
    if (after)
    {
        print_memory_dump(lines, false);
    }

    ofstream memdump_file;
    string memdump_file_name = string(argv[2]) + "_memdump.txt";
    memdump_file.open(memdump_file_name, ios::out);

    // printing mem dump to file
    print_memory_dump_to_file(memdump_file, lines);

    memdump_file.close();

    return 0;
}