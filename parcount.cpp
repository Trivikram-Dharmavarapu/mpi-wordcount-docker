#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <iomanip>
#include <sstream>

using namespace std;

#define ASCII_RANGE 128

#define DEBUG 0


void logs(int rank, const string& log) {
    if(DEBUG){
        cout << "[worker " << rank << "]: " << log << endl;
    }
}

void printCharacterFrequencies(long long int global_char_freq[]) {
    cout << "\n========= Top 10 Characters =========\n";
    cout << "Ch\tFreq\n";
    cout << "-------------------------------------\n";
    
    vector<pair<char, long long int>> sorted_chars;

    for (int i = 0; i < 128; i++) {
        if (global_char_freq[i] > 0) {
            sorted_chars.push_back({(char)i, global_char_freq[i]});
        }
    }

    sort(sorted_chars.begin(), sorted_chars.end(), [](const auto& a, const auto& b) {
        return b.second < a.second;
    });

    for (size_t i = 0; i < 10 && i < sorted_chars.size(); i++) {
        cout << sorted_chars[i].first << "\t" << sorted_chars[i].second << endl;
    }
}

void printWordFrequencies(const map<string, pair<long long int, long long int>>& word_map) {
    vector<pair<string, pair<long long int, long long int>>> sorted_words(word_map.begin(), word_map.end());

    sort(sorted_words.begin(), sorted_words.end(), [](const auto& a, const auto& b) {
        if (a.second.first != b.second.first)
            return a.second.first > b.second.first;
        return a.second.second < b.second.second;
    });

    cout << "\n=========== Top 10 Words ============\n";
    cout << left << setw(15) << "Word" << "\tID\tFreq\n";
    cout << "-------------------------------------\n";

    int count = 0;
    for (const auto& word : sorted_words) {
        cout << left << setw(15) << word.first << "\t" << word.second.second << "\t" << word.second.first << endl;
        if (++count == 10) break;
    }
}

void sendIncompleteWord(const string& word, int rank, int world_size) {
    if (rank < world_size - 1) {
        MPI_Send(word.c_str(), word.length() + 1, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
        logs(rank, "Sent incomplete word to next rank: " + word);
    } else if (rank == world_size - 1) {
        logs(rank, "Adding last incomplete word to local frequency: " + word);
    }
}

void handleStartWord(const char* buffer, MPI_Offset& process_start, MPI_Offset segment_size, string& current_word, long long int& word_id_counter, long long int& start_word_id, int rank, long long int local_char_freq[]) {
    start_word_id = word_id_counter++;
    while (process_start < segment_size && isalnum(buffer[process_start])) {
        unsigned char c = (unsigned char)buffer[process_start];
        if (c >= 32 && c <= 126 && c != 127) {
            local_char_freq[c]++;
        }
        current_word += tolower(buffer[process_start]);
        process_start++;
    }
    logs(rank, "Processed starting word: " + current_word);
}

void addWordToFrequencyMap(string& word, map<string, pair<long long int, long long int>>& local_word_freq, long long int& word_id_counter) {
    if (!word.empty()) {
        if (local_word_freq.find(word) == local_word_freq.end()) {
            local_word_freq[word] = {1, word_id_counter++};
        } else {
            local_word_freq[word].first++;
        }
        word = "";
    }
}

void processBuffer(const char* buffer, MPI_Offset process_start, MPI_Offset segment_size, long long int local_char_freq[], map<string, pair<long long int, long long int>>& local_word_freq, long long int& word_id_counter, string& current_word) {
    for (MPI_Offset i = process_start; i < segment_size; i++) {
        unsigned char c = (unsigned char)buffer[i];
        if (c >= 32 && c <= 126 && c != 127) {
            local_char_freq[c]++;
        }
        if (isalnum(c)) {
            current_word += tolower(c);
        } else {
            if (!current_word.empty()) {
                addWordToFrequencyMap(current_word, local_word_freq, word_id_counter);
            }
        }
    }
}

void receiveIncompleteWordAsync(char* received_word, MPI_Request& request, int rank) {
    if (rank > 0) {
        MPI_Irecv(received_word, 256, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, &request);
        logs(rank, "Started non-blocking receive for incomplete word.");
    }
}

void processTextData(MPI_File file, MPI_Offset start, MPI_Offset end, int rank, int world_size, long long int local_char_freq[], map<string, pair<long long int, long long int>>& local_word_freq) {
    MPI_Offset segment_size = end - start;
    char* buffer = (char*)malloc(segment_size + 1);
    char received_word[256] = "";

    if (buffer == NULL) {
        logs(rank, "Failed to allocate memory for buffer");
        return;
    }

    MPI_File_read_at(file, start, buffer, segment_size, MPI_BYTE, MPI_STATUS_IGNORE);
    buffer[segment_size] = '\0';

    logs(rank, "Successfully read data from file.");

    // Receive patian word if any
    MPI_Request request;
    if (rank > 0) {
        receiveIncompleteWordAsync(received_word, request, rank);
    }

    long long int word_id_counter = (rank * segment_size) + 1 ;
    MPI_Offset process_start = 0;

    logs(rank, "word_id_counter" + to_string(word_id_counter));

    string start_word = "";
    long long int word_id_start = word_id_counter;
    MPI_Offset current_position = 0;
    logs(rank, "word_id_start" + to_string(word_id_start));

    if(rank > 0) {
        handleStartWord(buffer, current_position, segment_size, start_word, word_id_counter, word_id_start, rank, local_char_freq);
    }

    string current_word = "";
    processBuffer(buffer, current_position, segment_size, local_char_freq, local_word_freq, word_id_counter, current_word);

    // Send the partial word any
    sendIncompleteWord(current_word, rank, world_size);

    if (rank > 0) {
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        logs(rank, "Received incomplete word: " + string(received_word));

        // Concatenate the received partial word
        if (strlen(received_word) > 0) {
            start_word = string(received_word) + start_word;
            logs(rank, "Concatenated incomplete word: " + start_word);
        }
    }

    logs(rank, "Adding start word with ID: " + to_string(word_id_start));
    addWordToFrequencyMap(start_word, local_word_freq, word_id_counter);

    free(buffer);
    logs(rank, "Completed processing text data.");
}

string serializeWordMap(const map<string, pair<long long int, long long int>>& word_map) {
    stringstream serialized_data;
    for (const auto& entry : word_map) {
        serialized_data << entry.first << ":" << entry.second.first << ":" << entry.second.second << "\n";
    }
    return serialized_data.str();
}

void deserializeWordMap(const string& serialized_data, map<string, pair<long long int, long long int>>& word_map) {
    stringstream ss(serialized_data);
    string line;
    while (getline(ss, line)) {
        size_t first_colon = line.find(":");
        size_t second_colon = line.find(":", first_colon + 1);
        
        string word = line.substr(0, first_colon);
        long long int freq = stoll(line.substr(first_colon + 1, second_colon - first_colon - 1));
        long long int id = stoll(line.substr(second_colon + 1));
        
        if (word_map.find(word) != word_map.end()) {
            word_map[word].first += freq;
        } else {
            word_map[word] = {freq, id};
        }
    }
}

void mergeWordMaps(map<string, pair<long long int, long long int>>& global_word_map,
                   const map<string, pair<long long int, long long int>>& local_word_map) {
    for (const auto& entry : local_word_map) {
        const string& word = entry.first;
        long long int local_freq = entry.second.first;
        long long int local_id = entry.second.second;
        
        if (global_word_map.find(word) == global_word_map.end()) {
            global_word_map[word] = {local_freq, local_id};
        } else {
            global_word_map[word].first += local_freq;
        }
    }
}

int main(int argc, char** argv) {

    clock_t start_time = clock();

    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    if (argc < 2) {
        if (world_rank == 0) {
            fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    string input_file = argv[1];

    MPI_File file;
    MPI_Offset file_size;
    MPI_File_open(MPI_COMM_WORLD, input_file.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    MPI_File_get_size(file, &file_size);

    MPI_Bcast(&file_size, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    MPI_Offset segment_size = file_size / world_size;
    MPI_Offset segment_start = world_rank * segment_size;
    MPI_Offset segment_end = (world_rank == world_size - 1) ? file_size : (world_rank + 1) * segment_size;

    map<string, pair<long long int, long long int>> local_word_freq;
    long long int word_id_counter = 1;

    long long int local_char_freq[ASCII_RANGE] = {0};
    long long int global_char_freq[ASCII_RANGE] = {0};

    // Process file data
    processTextData(file, segment_start, segment_end, world_rank, world_size, local_char_freq, local_word_freq);

    MPI_File_close(&file);

    MPI_Allreduce(local_char_freq, global_char_freq, ASCII_RANGE, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    logs(world_rank, "MPI_Allreduce for char frequencies completed.");

    string serialized_local_word_freq = serializeWordMap(local_word_freq);
    int local_word_data_size = serialized_local_word_freq.size();
    
    vector<int> all_word_data_sizes(world_size);
    MPI_Gather(&local_word_data_size, 1, MPI_INT, all_word_data_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    logs(world_rank, "MPI_Gather for word data sizes completed.");

    vector<int> displacements(world_size, 0);
    long long int total_word_data_size = 0;
    if (world_rank == 0) {
        for (int i = 0; i < world_size; i++) {
            displacements[i] = total_word_data_size;
            total_word_data_size += all_word_data_sizes[i];
        }
        logs(world_rank, "Total word data size: " + to_string(total_word_data_size));
    }

    char* global_serialized_word_freq = nullptr;
    if (world_rank == 0 && total_word_data_size > 0) {
        global_serialized_word_freq = (char*)malloc(total_word_data_size);
        if (!global_serialized_word_freq) {
            logs(world_rank, "Failed to allocate memory for global_serialized_word_freq");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        memset(global_serialized_word_freq, 0, total_word_data_size);
    }

    MPI_Gatherv(serialized_local_word_freq.c_str(), local_word_data_size, MPI_CHAR,
                global_serialized_word_freq, all_word_data_sizes.data(), displacements.data(),
                MPI_CHAR, 0, MPI_COMM_WORLD);
    logs(world_rank, "MPI_Gatherv completed.");

    map<string, pair<long long int, long long int>> global_word_freq;
    if (world_rank == 0 && global_serialized_word_freq != nullptr) {
        string global_data(global_serialized_word_freq, total_word_data_size);
        deserializeWordMap(global_data, global_word_freq);
        free(global_serialized_word_freq);

        printCharacterFrequencies(global_char_freq);
        printWordFrequencies(global_word_freq);
    }

    double time_taken = ((double)(clock() - start_time)) / CLOCKS_PER_SEC;
    if (world_rank == 0) {
        printf("Total execution time: %f seconds\n", time_taken);
    }

    MPI_Finalize();
    return 0;
}
