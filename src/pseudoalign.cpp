#include <string>
#include <sstream>
#include "pseudoalign.hh"
#include "WorkDispatcher.hh"

using namespace std;
using namespace sbwt;

vector<vector<int64_t> > parse_pseudoalignment_output_format_from_disk(string filename){
    vector<pair<int64_t, vector<int64_t>>> results; // Pairs (query id, color set)
    check_readable(filename);
    throwing_ifstream input(filename);
    string line;

    int64_t line_number = 0;
    while(input.getline(line)){
        vector<int64_t> tokens = parse_tokens<int64_t>(line);
        assert(tokens.size() >= 1);
        int64_t query_id = tokens[0];
        vector<int64_t> alignments;
        for(int64_t i = 1; i < tokens.size(); i++){
            alignments.push_back(tokens[i]);
        }
        results.push_back({query_id, alignments});
        line_number++;
    }

    sort(results.begin(), results.end());
    vector<vector<int64_t> > just_color_sets;
    for(auto X : results) just_color_sets.push_back(X.second);
    return just_color_sets;
}


// If outfile is empty, creates a writer to cout
std::unique_ptr<ParallelBaseWriter> create_writer(const string& outfile, bool gzipped){
    std::unique_ptr<ParallelBaseWriter> out = nullptr;
    if (!outfile.empty()) {
        if (gzipped)
            out = std::make_unique<ParallelGzipWriter>(outfile);
        else
            out = std::make_unique<ParallelOutputWriter>(outfile);
    } else {
        if (gzipped)
            out = std::make_unique<ParallelGzipWriter>(cout);
        else
            out = std::make_unique<ParallelOutputWriter>(cout);
    }
    return out;
}

void call_sort_parallel_output_file(const string& outfile, bool gzipped){
    if(outfile == "") throw std::runtime_error("Can't sort stdout output");

    write_log("Sorting output file", LogLevel::MAJOR);
    string tempfile = get_temp_file_manager().create_filename("results_temp");
    if (gzipped) {
        zstr::ifstream instream(outfile);
        zstr::ofstream outstream(tempfile);
        sort_parallel_output_file(instream, outstream);
    } else {
        throwing_ifstream instream(outfile);
        throwing_ofstream outstream(tempfile);
        sort_parallel_output_file(instream.stream, outstream.stream);
    }
    std::filesystem::rename(tempfile, outfile);
}