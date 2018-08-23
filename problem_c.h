#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <unordered_set>
#include <unordered_map>

typedef struct line {
    int left; // bottom
    int right; // top
} Line;

typedef struct rect {
    int bl_x;
    int bl_y;
    int tr_x;
    int tr_y;
    int width_x;
    int width_y;
    int near_critical;
} Rect;

typedef struct layout {
    int id;
    int bl_x;
    int bl_y;
    int tr_x;
    int tr_y;
    int net_id;
    int layer;
    // Drv_Pin 0 | Normal 1 | Load_Pin 2 | Fill 3
    int type;
    int is_critical;
} Layout;

typedef struct rule {
    int layer;
    int min_width;
    int min_space;
    int max_fill_width;
    double min_density;
    double max_density;
} Rule;

// class Metal {
// public:
//     int id;
//     Metal *next_x;
//     Metal *next_y;

//     Metal() {
//         id = 0;
//         next_x = NULL;
//         next_y = NULL;
//     };

//     Metal(int id) {
//         this->id = id;
//         next_x = NULL;
//         next_y = NULL;
//     }

//     Metal(int id, mMetal *next_x, Metal *next_y) {
//         this->id = id;
//         this->next_x = next_x;
//         this->next_y = next_y;
//     }
// };

typedef struct quarter_window {
    int index;
    long long area;
    // coordinate
    int bl_x;
    int bl_y;
    int tr_x;
    int tr_y;
    std::vector<int> contribute_metals;
    int violate_count;
    std::vector<int> affected_window;
} QuarterWindow;

typedef struct window {
    int index;
    long long area;
    long long area_insufficient;
    double density;
    std::vector<int> included_qwindow;
} Window;

typedef struct area_table {
    std::vector<double> s;
    std::vector<double> a;
    std::vector<double> b;
} AreaTable;

typedef struct fringe_table {
    std::vector<double> d;
    std::vector<double> a;
    std::vector<double> b;
} FringeTable; // including special case lateral

std::string path;

// config file
std::string config_file, circuit_file, output_file, process_file, rule_file;

// parameters
int total_metals;
int total_layers;
int total_fills = 0;

// hash set
std::unordered_set<int> critical_nets;
int power_net, ground_net;

// circuit file
Rect cb; // chip boundary
std::vector<Layout> layouts;
std::vector<Layout> metal_fill_layouts;

// process file
int window_size = 10000;
int stride;

// total windows and quarter windows
int window_x, window_y;
int qwindow_x, qwindow_y;

// rule file
std::vector<Rule> rules;
std::vector<long long> min_area_per_window;

// metal linked list, head nodes of each layer
// std::vector<Metal*> metals;

// 1/4 window
std::vector<std::vector<QuarterWindow>> quarter_windows;

// sliding window
std::vector<std::vector<Window>> windows;

// capacitance tables
std::vector<std::string> area_tables;
std::vector<std::string> fringe_tables;
std::map<std::string, AreaTable> area_table_map;
std::map<std::string, FringeTable> fringe_table_map; // incuding lateral table

// coupling capacitance
std::unordered_map<long long, double> cap;
