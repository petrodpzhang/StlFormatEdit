#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.stl> [index1 index2 ...]" << endl;
        cerr << "Example: " << argv[0] << " input.stl 0 2 5" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = "output.stl";
    string solidName;

    vector<Facet> facets;
    readSTL(inputFile, facets, solidName);

    cout << "Total facets: " << facets.size() << endl;
    cout << "Facet indices: 0 to " << facets.size() - 1 << endl;

    if (argc > 2) {
        vector<int> indicesToDelete;
        for (int i = 2; i < argc; ++i) {
            int index = atoi(argv[i]);
            indicesToDelete.push_back(index);
        }

        deleteFacets(facets, indicesToDelete);
        cout << "Deleted " << indicesToDelete.size() << " facets. Remaining: " << facets.size() << endl;
    }

    writeSTL(outputFile, facets, solidName);
    cout << "New STL file written to: " << outputFile << endl;

    return 0;
}