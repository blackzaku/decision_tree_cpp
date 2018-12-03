#include <iostream>
#include "csv_reader.h"
#include "decision_tree.h"

int main(int argc, char *argv[], char *envp[] ) {

    if (argc < 3) {
        std::cout << "Usage: dectree input-file output-attribute-index (separator=';') (has header=1)" << std::endl;
    } else {
        // Read filename and attribute index
        std::string file_name(argv[1]);
        std::string attribute_index(argv[2]);
        char separator = ';';
        bool has_header = true;

        // Value splitting character
        if (argc >= 4) {
            separator = argv[3][0];
        }

        // First line represents the header
        if (argc >= 5) {
            separator = argv[4][0] == '1';
        }

        // Read the CSV file
        csv_reader csv(file_name, separator, has_header);

        // Build the tree
        decision_tree tree(csv, std::stoi(attribute_index));

        // Print the tree
        tree.print(csv);

        return 0;
    }
}