// -*- coding: utf-8 -*-
#pragma execution_character_set("utf-8")

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstdlib>

// CSV row structure
struct CSVRow {
    std::vector<std::string> fields;
};

// CSV file data structure
struct CSVData {
    std::vector<std::string> headers;
    std::vector<CSVRow> rows;
};

// Read CSV file (similar style to read_order_file)
void read_csv_file(const std::string& filename, CSVData& csv_data) {
    // Try multiple paths
    std::vector<std::string> paths_to_try;
    paths_to_try.push_back(filename);
    paths_to_try.push_back("../" + filename);
    paths_to_try.push_back("../../" + filename);
    paths_to_try.push_back("../../../" + filename);
    paths_to_try.push_back("../../../../" + filename);
    
    std::ifstream file;
    std::string successful_path = "";
    
    for (size_t i = 0; i < paths_to_try.size(); i++) {
        file.open(paths_to_try[i].c_str());
        if (file.is_open()) {
            successful_path = paths_to_try[i];
            std::cout << "Successfully opened: " << successful_path << std::endl;
            break;
        }
    }
    
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << " (tried " << paths_to_try.size() << " paths)" << std::endl;
        return;
    }
    
    std::string line;
    // Read header
    if (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::string field = "";
        
        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];
            if (c == ',' || c == '\r') {
                if (c == ',') {
                    fields.push_back(field);
                    field = "";
                }
            } else {
                field += c;
            }
        }
        fields.push_back(field);  // Last field
        csv_data.headers = fields;
        
        std::cout << "Header loaded from " << filename << ": ";
        for (size_t i = 0; i < csv_data.headers.size(); i++) {
            std::cout << csv_data.headers[i];
            if (i < csv_data.headers.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    // Read data rows
    int line_num = 1;
    while (std::getline(file, line)) {
        line_num++;
        if (line.empty()) continue;
        
        try {
            std::vector<std::string> fields;
            std::string field = "";
            
            for (size_t i = 0; i < line.length(); i++) {
                char c = line[i];
                if (c == ',' || c == '\r') {
                    if (c == ',') {
                        fields.push_back(field);
                        field = "";
                    }
                } else {
                    field += c;
                }
            }
            fields.push_back(field);  // Last field
            
            CSVRow row;
            row.fields = fields;
            csv_data.rows.push_back(row);
            
        } catch (const std::exception& e) {
            std::cout << "Error parsing line " << line_num << ": " << e.what() << std::endl;
            std::cout << "Line content: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "Read " << csv_data.rows.size() << " rows from " << filename << std::endl;
}

// ========== Mode 1: Full comparison of two CSV files ==========
void compareMode1() {
    std::cout << "\n========== Mode 1: Full comparison of two CSV files ==========" << std::endl;
    
    CSVData csv1, csv2;
    
    read_csv_file("sample1.csv", csv1);
    read_csv_file("sample2.csv", csv2);
    
    if (csv1.rows.empty() || csv2.rows.empty()) {
        std::cout << "Error: One or both files are empty or could not be read" << std::endl;
        return;
    }
    
    std::cout << "\nFile 1 (sample1.csv) rows: " << csv1.rows.size() << std::endl;
    std::cout << "File 2 (sample2.csv) rows: " << csv2.rows.size() << std::endl;
    
    // Compare headers
    std::cout << "\n--- Header Comparison ---" << std::endl;
    if (csv1.headers == csv2.headers) {
        std::cout << "Headers are the same" << std::endl;
    } else {
        std::cout << "Headers are different!" << std::endl;
        std::cout << "File 1 headers: ";
        for (size_t i = 0; i < csv1.headers.size(); i++) {
            std::cout << csv1.headers[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "File 2 headers: ";
        for (size_t i = 0; i < csv2.headers.size(); i++) {
            std::cout << csv2.headers[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Store csv1 data in map, using first column as key
    std::map<std::string, std::vector<std::string> > map1;
    for (size_t i = 0; i < csv1.rows.size(); i++) {
        if (!csv1.rows[i].fields.empty()) {
            map1[csv1.rows[i].fields[0]] = csv1.rows[i].fields;
        }
    }
    
    // Store csv2 data in map
    std::map<std::string, std::vector<std::string> > map2;
    for (size_t i = 0; i < csv2.rows.size(); i++) {
        if (!csv2.rows[i].fields.empty()) {
            map2[csv2.rows[i].fields[0]] = csv2.rows[i].fields;
        }
    }
    
    // Find rows in csv1 but not in csv2
    std::cout << "\n--- Records in File 1 but not in File 2 ---" << std::endl;
    int count1 = 0;
    for (std::map<std::string, std::vector<std::string> >::iterator it = map1.begin(); 
         it != map1.end(); ++it) {
        if (map2.find(it->first) == map2.end()) {
            std::cout << "ID: " << it->first << " -> ";
            for (size_t i = 0; i < it->second.size(); i++) {
                std::cout << it->second[i] << " ";
            }
            std::cout << std::endl;
            count1++;
        }
    }
    if (count1 == 0) std::cout << "None" << std::endl;
    
    // Find rows in csv2 but not in csv1
    std::cout << "\n--- Records in File 2 but not in File 1 ---" << std::endl;
    int count2 = 0;
    for (std::map<std::string, std::vector<std::string> >::iterator it = map2.begin(); 
         it != map2.end(); ++it) {
        if (map1.find(it->first) == map1.end()) {
            std::cout << "ID: " << it->first << " -> ";
            for (size_t i = 0; i < it->second.size(); i++) {
                std::cout << it->second[i] << " ";
            }
            std::cout << std::endl;
            count2++;
        }
    }
    if (count2 == 0) std::cout << "None" << std::endl;
    
    // Find common rows with different content
    std::cout << "\n--- Records in both files but with different content ---" << std::endl;
    int count3 = 0;
    for (std::map<std::string, std::vector<std::string> >::iterator it = map1.begin(); 
         it != map1.end(); ++it) {
        if (map2.find(it->first) != map2.end()) {
            const std::vector<std::string>& row1 = it->second;
            const std::vector<std::string>& row2 = map2[it->first];
            
            if (row1 != row2) {
                std::cout << "ID: " << it->first << std::endl;
                std::cout << "  File 1: ";
                for (size_t i = 0; i < row1.size(); i++) {
                    std::cout << row1[i] << " ";
                }
                std::cout << std::endl;
                std::cout << "  File 2: ";
                for (size_t i = 0; i < row2.size(); i++) {
                    std::cout << row2[i] << " ";
                }
                std::cout << std::endl;
                
                // List different columns in detail
                size_t maxCols = (row1.size() > row2.size()) ? row1.size() : row2.size();
                for (size_t i = 0; i < maxCols; i++) {
                    std::string val1 = (i < row1.size()) ? row1[i] : "(missing)";
                    std::string val2 = (i < row2.size()) ? row2[i] : "(missing)";
                    if (val1 != val2) {
                        std::string colName = (i < csv1.headers.size()) ? csv1.headers[i] : "Column";
                        std::cout << "    Different column [" << colName << "]: \"" << val1 << "\" vs \"" << val2 << "\"" << std::endl;
                    }
                }
                count3++;
            }
        }
    }
    if (count3 == 0) std::cout << "None" << std::endl;
    
    std::cout << "\n--- Comparison Summary ---" << std::endl;
    std::cout << "Records unique to File 1: " << count1 << std::endl;
    std::cout << "Records unique to File 2: " << count2 << std::endl;
    std::cout << "Records with different content: " << count3 << std::endl;
    
    int matchCount = 0;
    for (std::map<std::string, std::vector<std::string> >::iterator it = map1.begin(); 
         it != map1.end(); ++it) {
        if (map2.find(it->first) != map2.end() && it->second == map2[it->first]) {
            matchCount++;
        }
    }
    std::cout << "Perfectly matching records: " << matchCount << std::endl;
}

// ========== Mode 2: Compare three CSVs, check if partial columns from files 1 and 2 exist in file 3 ==========
void compareMode2() {
    std::cout << "\n========== Mode 2: Three-file partial column comparison ==========" << std::endl;
    
    CSVData csv1, csv2, csv3;
    
    read_csv_file("sample1.csv", csv1);
    read_csv_file("sample2.csv", csv2);
    read_csv_file("sample3.csv", csv3);
    
    if (csv1.rows.empty() || csv2.rows.empty() || csv3.rows.empty()) {
        std::cout << "Error: One or more files are empty or could not be read" << std::endl;
        return;
    }
    
    std::cout << "\nFile 1 rows: " << csv1.rows.size() << std::endl;
    std::cout << "File 2 rows: " << csv2.rows.size() << std::endl;
    std::cout << "File 3 rows: " << csv3.rows.size() << std::endl;
    
    // Set column indices to compare (here we compare first 2 columns, you can modify as needed)
    std::vector<int> compareColumns;
    compareColumns.push_back(0);  // Compare column 0
    compareColumns.push_back(1);  // Compare column 1
    
    std::cout << "\nCompare column indices: ";
    for (size_t i = 0; i < compareColumns.size(); i++) {
        std::cout << compareColumns[i] << " ";
    }
    std::cout << std::endl;
    
    // Store specified column combinations from csv3 into set
    std::set<std::string> set3;
    for (size_t i = 0; i < csv3.rows.size(); i++) {
        std::string key = "";
        for (size_t j = 0; j < compareColumns.size(); j++) {
            int col = compareColumns[j];
            if (col < (int)csv3.rows[i].fields.size()) {
                if (!key.empty()) key += "|";
                key += csv3.rows[i].fields[col];
            }
        }
        if (!key.empty()) {
            set3.insert(key);
        }
    }
    
    std::cout << "\nUnique combinations in File 3: " << set3.size() << std::endl;
    
    // Check data from csv1
    std::cout << "\n--- Checking records from File 1 ---" << std::endl;
    int found1 = 0, notfound1 = 0;
    for (size_t i = 0; i < csv1.rows.size(); i++) {
        std::string key = "";
        for (size_t j = 0; j < compareColumns.size(); j++) {
            int col = compareColumns[j];
            if (col < (int)csv1.rows[i].fields.size()) {
                if (!key.empty()) key += "|";
                key += csv1.rows[i].fields[col];
            }
        }
        
        if (set3.find(key) != set3.end()) {
            std::cout << "  [FOUND] Found in File 3: " << key << std::endl;
            found1++;
        } else {
            std::cout << "  [NOT FOUND] Not found in File 3: " << key << std::endl;
            notfound1++;
        }
    }
    
    // Check data from csv2
    std::cout << "\n--- Checking records from File 2 ---" << std::endl;
    int found2 = 0, notfound2 = 0;
    for (size_t i = 0; i < csv2.rows.size(); i++) {
        std::string key = "";
        for (size_t j = 0; j < compareColumns.size(); j++) {
            int col = compareColumns[j];
            if (col < (int)csv2.rows[i].fields.size()) {
                if (!key.empty()) key += "|";
                key += csv2.rows[i].fields[col];
            }
        }
        
        if (set3.find(key) != set3.end()) {
            std::cout << "  [FOUND] Found in File 3: " << key << std::endl;
            found2++;
        } else {
            std::cout << "  [NOT FOUND] Not found in File 3: " << key << std::endl;
            notfound2++;
        }
    }
    
    std::cout << "\n--- Comparison Summary ---" << std::endl;
    std::cout << "File 1: Found " << found1 << ", Not found " << notfound1 << std::endl;
    std::cout << "File 2: Found " << found2 << ", Not found " << notfound2 << std::endl;
}

// ========== Main Function ==========
int main(int argc, char** argv) {
    std::cout << "CSV File Comparison Tool" << std::endl;
    std::cout << "================================" << std::endl;
    
    // ========================================
    // Uncomment one of the lines below to select comparison mode
    // ========================================
    
    compareMode1();  // Mode 1: Full comparison of two CSV files
    // compareMode2();  // Mode 2: Three-file partial column comparison
    
    std::cout << "\nProgram completed!" << std::endl;
    return 0;
}
