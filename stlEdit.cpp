#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <unordered_set>

using namespace std;

struct Vertex {
    float x, y, z;
};

struct Facet {
    Vertex normal;
    Vertex v1, v2, v3;
    string extra; // for any extra info
};

void readSTL(const string& filename, vector<Facet>& facets, string& solidName) {
    ifstream file(filename.c_str());
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    string line;
    getline(file, line);
    
    // Extract solid name
    size_t solidPos = line.find("solid ");
    if (solidPos != string::npos) {
        solidName = line.substr(solidPos + 6);
    }

    Facet currentFacet;
    int vertexCount = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        string keyword;
        ss >> keyword;

        if (keyword == "facet") {
            string normal;
            ss >> normal >> currentFacet.normal.x >> currentFacet.normal.y >> currentFacet.normal.z;
            vertexCount = 0;
            currentFacet.extra = line;
        }
        else if (keyword == "vertex") {
            Vertex v;
            ss >> v.x >> v.y >> v.z;
            if (vertexCount == 0) currentFacet.v1 = v;
            else if (vertexCount == 1) currentFacet.v2 = v;
            else if (vertexCount == 2) currentFacet.v3 = v;
            vertexCount++;
        }
        else if (keyword == "endfacet") {
            facets.push_back(currentFacet);
        }
    }

    file.close();
}

void writeSTL(const string& filename, const vector<Facet>& facets, const string& solidName) {
    ofstream file(filename.c_str());
    if (!file.is_open()) {
        cerr << "Error creating file: " << filename << endl;
        exit(1);
    }

    file << "solid " << solidName << endl;

    for (size_t i = 0; i < facets.size(); ++i) {
        const Facet& f = facets[i];
        file << "  facet normal " << f.normal.x << " " << f.normal.y << " " << f.normal.z << endl;
        file << "    outer loop" << endl;
        file << "      vertex " << f.v1.x << " " << f.v1.y << " " << f.v1.z << endl;
        file << "      vertex " << f.v2.x << " " << f.v2.y << " " << f.v2.z << endl;
        file << "      vertex " << f.v3.x << " " << f.v3.y << " " << f.v3.z << endl;
        file << "    endloop" << endl;
        file << "  endfacet" << endl;
    }

    file << "endsolid " << solidName << endl;
    file.close();
}

void deleteFacets(vector<Facet>& facets, const vector<int>& indicesToDelete) {
    // Sort indices in descending order for safe deletion
    vector<int> sortedIndices = indicesToDelete;
    sort(sortedIndices.rbegin(), sortedIndices.rend());

    // Remove duplicates
    sortedIndices.erase(unique(sortedIndices.begin(), sortedIndices.end()), sortedIndices.end());

    // Delete facets
    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        int index = sortedIndices[i];
        if (index >= 0 && index < (int)facets.size()) {
            facets.erase(facets.begin() + index);
        }
    }
}

vector<int> findFacetsAtZValues(const vector<Facet>& facets, const vector<float>& zValues, float tolerance = 1e-6) {
    vector<int> indices;
    unordered_set<int> uniqueIndices; // To avoid duplicates
    
    for (size_t i = 0; i < facets.size(); ++i) {
        const Facet& f = facets[i];
        
        // Check if the facet is parallel to XY plane (normal is (0,0,1) or (0,0,-1))
        if (fabs(f.normal.x) < tolerance && fabs(f.normal.y) < tolerance && fabs(fabs(f.normal.z) - 1.0) < tolerance) {
            // Check if all three vertices are at any of the specified Z values
            for (float zValue : zValues) {
                if (fabs(f.v1.z - zValue) < tolerance && 
                    fabs(f.v2.z - zValue) < tolerance && 
                    fabs(f.v3.z - zValue) < tolerance) {
                    if (uniqueIndices.find(i) == uniqueIndices.end()) {
                        indices.push_back(i);
                        uniqueIndices.insert(i);
                    }
                    break; // No need to check other Z values for this facet
                }
            }
        }
    }
    
    return indices;
}

vector<float> parseZValues(int argc, char* argv[], int startIndex) {
    vector<float> zValues;
    for (int i = startIndex; i < argc; ++i) {
        char* end;
        float z = strtof(argv[i], &end);
        if (end != argv[i]) { // Successful conversion
            zValues.push_back(z);
        } else {
            cerr << "Warning: Invalid Z value '" << argv[i] << "' ignored." << endl;
        }
    }
    return zValues;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.stl> <z_value1> [z_value2 ...] [tolerance]" << endl;
        cerr << "Example: " << argv[0] << " input.stl 10.0 20.5 30.0 0.001" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = "output.stl";
    string solidName;

    // Parse Z values and tolerance
    float tolerance = 1e-6f;
    vector<float> zValues = parseZValues(argc, argv, 2);
    
    // Check if last argument is tolerance (must be after at least one Z value)
    if (zValues.size() > 0 && argc > zValues.size() + 2) {
        char* end;
        tolerance = strtof(argv[argc-1], &end);
        if (end == argv[argc-1]) { // Conversion failed
            tolerance = 1e-6f;
            cerr << "Warning: Invalid tolerance value, using default 1e-6" << endl;
        } else {
            // Remove tolerance from zValues if it was mistakenly added
            if (!zValues.empty() && fabs(zValues.back() - tolerance) < 1e-12) {
                zValues.pop_back();
            }
        }
    }

    if (zValues.empty()) {
        cerr << "Error: No valid Z values provided" << endl;
        return 1;
    }

    vector<Facet> facets;
    readSTL(inputFile, facets, solidName);

    cout << "Total facets: " << facets.size() << endl;
    cout << "Z values to delete: ";
    for (float z : zValues) cout << z << " ";
    cout << "\nTolerance: " << tolerance << endl;

    // Find facets at the specified Z values
    vector<int> indicesToDelete = findFacetsAtZValues(facets, zValues, tolerance);
    
    if (!indicesToDelete.empty()) {
        deleteFacets(facets, indicesToDelete);
        cout << "Deleted " << indicesToDelete.size() << " facets at specified Z values. Remaining: " << facets.size() << endl;
    } else {
        cout << "No facets found at specified Z values with tolerance " << tolerance << endl;
    }

    writeSTL(outputFile, facets, solidName);
    cout << "New STL file written to: " << outputFile << endl;

    return 0;
}