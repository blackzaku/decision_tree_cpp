#include <utility>

//
// Created by smedina on 12/1/18.
//

#ifndef ADULTS_CPP_CSV_READER_H
#define ADULTS_CPP_CSV_READER_H

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>

class csv_reader {
private:
    char separator = ';';
    std::vector<int> enabled_rows;
    std::vector<bool> enabled_columns;
    std::vector<std::string> *column_names;
    std::vector<std::map<std::string, int>> *values_to_idx;
    std::vector<std::vector<int>> *values;

    std::vector<std::string> read_line(std::string line) {
        std::vector<std::string> row;
        auto index = line.find_first_of(separator);
        while (index != std::string::npos) {
            row.push_back(line.substr(0, index));
            line = line.substr(index + 1);
            index = line.find_first_of(separator);
        }
        row.push_back(line);
        return row;
    }

    void add_row(std::string const &line_string) {
        auto row_string = read_line(line_string);
        std::vector<int> row(row_string.size());
        for (int i = 0; i < row_string.size(); i += 1) {
            row[i] = row_to_idx(i, row_string[i]);
        }
        enabled_rows.emplace_back(values->size());
        values->emplace_back(row);
    }

    int row_to_idx(int column_index, std::string const &value) {
        auto &value_to_idx = (*values_to_idx)[column_index];
        auto found = value_to_idx.find(value);
        if (found != value_to_idx.end()) {
            return found->second;
        } else {
            auto vectorized = (int) value_to_idx.size();
            value_to_idx[value] = vectorized;
            return vectorized;
        }
    }

    void set_header(std::string const &line_string, bool anonymous_names = false) {
        auto row_string = read_line(line_string);
        for (int i = 0; i < row_string.size(); i += 1) {
            if (anonymous_names) {
                column_names->emplace_back("Parameter " + std::to_string(i));
            } else {
                column_names->emplace_back(row_string[i]);
            }

            enabled_columns.emplace_back(true);
            values_to_idx->emplace_back(std::map<std::string, int>());
        }
    }

    csv_reader(char separator,
               std::vector<int> enabled_rows,
               std::vector<bool> enabled_columns,
               std::vector<std::string> *column_names,
               std::vector<std::map<std::string, int>> *values_to_idx,
               std::vector<std::vector<int>> *values) :
            separator(separator),
            enabled_rows(std::move(enabled_rows)),
            enabled_columns(std::move(enabled_columns)),
            column_names(column_names),
            values_to_idx(values_to_idx),
            values(values) {

    }

public:

    csv_reader(std::string const &file_name,
            char separator=';',
            bool first_line_header=true) : separator(separator) {
        column_names = new std::vector<std::string>();
        values_to_idx = new std::vector<std::map<std::string, int>>();
        values = new std::vector<std::vector<int>>();
        auto file = std::ifstream(file_name);
        std::string line;
        // Read the header
        std::getline(file, line);
        if (first_line_header) {
            auto first_index = line[0] == '#'? line.find_first_not_of(' ', 1) : 0;
            // It is a header
            line = line.substr(first_index);
            set_header(line);
        } else {
            // It is a row, add it
            set_header(line, true);
            add_row(line);
        }
        // Read the rest of the lines
        while (std::getline(file, line)) {
            add_row(line);
        }
    }

    void disable_column(int column_index) {
        enabled_columns[column_index] = false;
    }

    void print(std::ostream &output = std::cout) {
        output << "# ";
        for (auto const &column_name: *column_names) output << column_name << ";";
        output << std::endl;
        for (auto i: enabled_rows) {
            for (auto const &value: (*values)[i]) output << std::to_string(value) << ";";
            output << std::endl;
        }
    }

    csv_reader filter(int column_index, int value, bool disable_column = true) {
        std::vector<int> enabled_rows;
        std::vector<bool> enabled_columns(this->enabled_columns);
        if (disable_column) enabled_columns[column_index] = false;

        for (auto i: this->enabled_rows) {
            if ((*values)[i][column_index] == value) {
                enabled_rows.push_back(i);
            }
        }
        csv_reader filtered(separator, enabled_rows, enabled_columns, column_names, values_to_idx, values);
        return filtered;
    }

    std::tuple<std::vector<int>, int> count_values(int column_index,
                                                   int conditional_column_index = -1,
                                                   int conditional_column_value = -1) {
        int total = 0;
        std::vector<int> counters((*values_to_idx)[column_index].size(), 0);
        for (auto i: enabled_rows) {
            if ((conditional_column_index == -1) ||
                (*values)[i][conditional_column_index] == conditional_column_value) {
                auto value = (*values)[i][column_index];
                total += 1;
                counters[value] += 1;
            }
        }
        return {counters, total};
    }

    bool all_equal() {
        auto all_counters = count_all();
        return all_equal(all_counters);
    }

    bool all_equal(std::vector<std::vector<int>> &all_counters) {
        for (auto counters: all_counters) {
            if (!all_counters.empty()) {
                auto already = false;
                for (auto counter: counters) {
                    if (counter > 0) {
                        if (already) return false;
                        else already = true;
                    }
                }
            }
        }
        return true;
    }

    std::vector<std::vector<int>> count_all() {
        std::vector<std::vector<int>> all_counters(enabled_columns.size());
        for (int j = 0; j < enabled_columns.size(); j += 1) {
            if (enabled_columns[j]) {
                all_counters[j] = std::vector<int>((*values_to_idx)[j].size());
            }
        }
        for (auto i: enabled_rows) {
            for (int j = 0; j < enabled_columns.size(); j += 1) {
                if (enabled_columns[j]) {
                    auto value = (*values)[i][j];
                    all_counters[j][value] += 1;
                }
            }
        }
        return all_counters;
    }

    std::vector<int> get_enabled_columns() {
        std::vector<int> column_indexes;
        for (int j = 0; j < enabled_columns.size(); j += 1) {
            if (enabled_columns[j]) column_indexes.emplace_back(j);
        }
        return column_indexes;
    }

    int enabled_rows_count() {
        return enabled_rows.size();
    }

    int enabled_columns_count() {
        int count = 0;
        for (int j = 0; j < enabled_columns.size(); j += 1) {
            if (enabled_columns[j]) count += 1;
        }
        return count;
    }

    std::string get_column_name(int column_index) {
        return (*column_names)[column_index];
    }

    std::string get_value_string(int column_index, int value_index) {
        for (auto const&[value_string, _value_index] : (*values_to_idx)[column_index]) {
            if (_value_index == value_index) return value_string;
        }
        throw "Invalid value index";
    }
};


#endif //ADULTS_CPP_CSV_READER_H
