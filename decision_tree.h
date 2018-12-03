//
// Created by smedina on 12/2/18.
//

#ifndef ADULTS_CPP_DECISION_TREE_H
#define ADULTS_CPP_DECISION_TREE_H

#include <vector>
#include <cmath>
#include <limits>
#include "csv_reader.h"

class decision_tree {
private:
    int x_decision_index = -1;
    std::vector<std::tuple<int, decision_tree>> children;
    int y_index = -1;
    int value = -1;

public:

    void print(csv_reader &csv, std::string parents="", std::ostream &output=std::cout) {
        if (value >= 0) {
            auto value_string = csv.get_value_string(y_index, value);
            auto column_name = csv.get_column_name(y_index);
            output << parents << " => " << column_name << " = " << value_string << std::endl;
        } else {
            if (not parents.empty()) {
                parents = parents + " & ";
            }
            for (auto &[value_index, sub_tree] : children) {
                auto value_string = csv.get_value_string(x_decision_index, value_index);
                auto column_name = csv.get_column_name(x_decision_index);
                auto parent_string = parents + column_name + " = " + value_string;
                sub_tree.print(csv, parent_string);
            }
        }
    }

    int max_index(std::vector<int> &counters) {
        int max_index = 0;
        int max_value = counters[0];
        for (int value_index = 1; value_index < counters.size(); value_index += 1) {
            if (counters[value_index] > max_value) max_index = value_index;
        }
        return max_index;
    }

    decision_tree(int value, int y_index, bool guess_from_parent=true) : value(value), y_index(y_index){

    }

    /**
     * Builds decision tree
     * @param csv Input CSV object
     * @param y_index Label index
     * @param guess_from_parent Guesses the leaves' class of unseen examples from the parent's labels
     */
    decision_tree(csv_reader &csv, int y_index, bool guess_from_parent=true) : value(-1), y_index(y_index) {
        csv.disable_column(y_index);
        // Check if all elements have the same class
        auto [counters, total] = csv.count_values(y_index);
        auto max_index = this->max_index(counters);
        if (counters[max_index] == total) {
            // All instances have the same class
            value = max_index;
        } else if (csv.all_equal()) {
            // All instances have the same values
            value = max_index;
        } else {
            auto [min_column, entropy] = min_conditional_entropy(csv);
            x_decision_index = min_column;
            auto [counters_x, total_x] = csv.count_values(x_decision_index);
            for (int value_index = 0; value_index < counters_x.size(); value_index += 1) {
                if (counters_x[value_index] > 0) {
                    auto filtered_csv = csv.filter(x_decision_index, value_index);
                    /*
                    std::cout << "compute decision_tree " << csv.get_column_name(x_decision_index) << "/" <<
                    csv.get_value_string(x_decision_index, value_index) << " -> " <<
                    std::to_string(filtered_csv.enabled_rows_count()) << std::endl;
                    */
                    // TODO: Call this in a pool of threads, there are no locks between calls
                    decision_tree child_decision_tree(filtered_csv, y_index, guess_from_parent);
                    children.emplace_back(std::tuple<int, decision_tree>(value_index, child_decision_tree));
                } else if (guess_from_parent) {
                    // We have to guess from the parent's majority
                    decision_tree child_decision_tree(max_index, y_index, guess_from_parent);
                    children.emplace_back(std::tuple<int, decision_tree>(value_index, child_decision_tree));
                }
            }
        }
    }

    double entropy(csv_reader& csv, int column_index) {
        auto [counters, total] = csv.count_values(column_index);
        return this->entropy(counters, total);
    }

    double entropy(std::vector<int> const& counters, int total) {
        double entropy = 0;
        for (int count: counters) {
            if (count > 0) {
                double probability = double(count) / double(total);
                entropy -= probability * std::log2(probability);
            }
        }
        return entropy;
    }


    double conditional_entropy(csv_reader& csv, int column_index) {
        double conditional_entropy = 0;
        auto [counters, total] = csv.count_values(column_index);
        for (int value_index = 0; value_index < counters.size(); value_index += 1) {
            auto [value_counters, value_total] = csv.count_values(y_index, column_index, value_index);
            auto entropy = this->entropy(value_counters, value_total);
            auto count_value_index = counters[value_index];
            if (count_value_index > 0) {
                double probability = double(count_value_index) / double(total);
                conditional_entropy += probability * entropy;
            }
        }
        return conditional_entropy;
    }

    std::tuple<int, double> min_conditional_entropy(csv_reader &csv) {
        auto enabled_columns = csv.get_enabled_columns();
        int min_index = 0;
        double min_entropy = std::numeric_limits<double>::max();
        for (auto column_index : enabled_columns) {
            auto entropy = conditional_entropy(csv, column_index);
            if (entropy < min_entropy) {
                min_index = column_index;
                min_entropy = entropy;
            }
        }
        return {min_index, min_entropy};
    }
};


#endif //ADULTS_CPP_DECISION_TREE_H
