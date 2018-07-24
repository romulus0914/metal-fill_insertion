#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>

//#include "problem_c.h"
#include "capacitance.h"

using namespace std;

void ReadConfig()
{
    ifstream file(config_file);

    string str;
    while (file >> str) {
        if (str == "design:")
            file >>  circuit_file;
        else if (str == "output:")
            file >> output_file;
        else if (str == "rule_file:")
            file >> rule_file;
        else if (str == "process_file:")
            file >> process_file;
        else if (str == "critical_nets:") {
            getline(file, str);
            char *ch = strtok((char *)str.c_str(), " ");
            while (ch != NULL) {
                critical_nets.insert(atoi(ch));
                ch = strtok(NULL, " ");
            }
        }
        else if (str == "power_nets:")
            file >> power_net;
        else if (str == "ground_net:")
            file >> ground_net;
    }

    file.close();
}

void ReadCircuit()
{
    ifstream file(path + circuit_file);

    string str;
    getline(file, str);
    size_t pos = str.find(" ");
    cb.bl_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.bl_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.tr_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(";");
    cb.tr_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    cb.width_x = cb.tr_x - cb.bl_x;
    cb.width_y = cb.tr_y - cb.bl_y;

    Layout l = {.id = 0, .bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, 
                .net_id = -1, .layer = -1, .type = -1, .isCritical = false};
    layouts.emplace_back(l); // dummy layout to align id and index;
    while (file >> l.id >> l.bl_x >> l.bl_y >> l.tr_x >> l.tr_y >> l.net_id >> l.layer >> str) {
        l.bl_x -= cb.bl_x;
        l.bl_y -= cb.bl_y;
        l.tr_x -= cb.bl_x;
        l.tr_y -= cb.bl_y;
        if (str == "Drv_Pin")
            l.type = 0;
        else if (str == "Normal" || str == "normal")
            l.type = 1;
        else if (str == "Load_Pin")
            l.type = 2;
        else if (str == "Fill")
            l.type = 3;
        else {
            cout << "Invalid Polygon Type: " << str << '\n';
            exit(1);
        }
        l.isCritical = critical_nets.find(l.net_id) != critical_nets.end();

        layouts.emplace_back(l);

    //     // get or create the head node of that layer
    //     if (metals.size() < l.layer) {
    //         Metal *head = new Metal(l.id);
    //         metals.emplace_back(head);
    //         continue;
    //     }
    //     Metal *metal = metals[l.layer - 1];

    //     Layout temp = layouts[metal->id];
    //     // sort order: x -> y
    //     // if x < head.x, replace head node
    //     if (l.bl_x < temp.bl_x) {
    //         Metal *new_head = new Metal(l.id);
    //         new_head->next_x = metal;
    //         metals[l.layer - 1] = new_head;
    //         continue;
    //     }
    //     // traverse in x direction
    //     Metal *prev = metal;
    //     while (metal != NULL) {
    //         temp = layouts[metal->id];
    //         if (l.bl_x <= temp.bl_x)
    //             break;
    //         prev = metal;
    //         metal = metal->next_x;
    //     }
    //     // traverse to last node or smaller means there is no exist node with that x
    //     if (metal == NULL || l.bl_x < temp.bl_x) {
    //         // prev -> new_metal -> metal
    //         Metal *new_metal = new Metal(l.id);
    //         prev->next_x = new_metal;
    //         new_metal->next_x = metal;
    //         continue;
    //     }
    //     // same x, traverse in y direction
    //     // if y < metal.y, replace y dircetion head node
    //     if (l.bl_y < temp.bl_y) {
    //         Metal *new_metal = new Metal(l.id);
    //         if (metal == metals[l.layer - 1]) {
    //             new_metal->next_x = metal->next_x;
    //             new_metal->next_y = metal;
    //             metal->next_x = NULL;
    //             metals[l.layer - 1] = new_metal;
    //         }
    //         else {
    //             // x direction link
    //             new_metal->next_x = metal->next_x;
    //             prev->next_x = new_metal;
    //             // y direction link
    //             metal->next_x = NULL;
    //             new_metal->next_y = metal;
    //         }
    //         continue;
    //     }
    //     prev = metal;
    //     // traverse in y direction
    //     while (metal != NULL) {
    //         temp = layouts[metal->id];
    //         if (l.bl_y < temp.bl_y) // impossible same y
    //             break;
    //         prev = metal;
    //         metal = metal->next_y;
    //     }
    //     // prev -> new_metal -> metal
    //     Metal *new_metal = new Metal(l.id);
    //     prev->next_y = new_metal;
    //     new_metal->next_y = metal;
    }

    file.close();
    
    total_metals = layouts.size() - 1;
    total_layers = layouts[total_metals - 1].layer;
}

void ReadProcess()
{
    ifstream file(path + process_file);

    string str;
    getline(file, str); // comment line
    getline(file, str);
    window_size = atoi((char *)str.substr(str.find(" ") + 1, str.length()).c_str());
    getline(file, str); // comment line
    getline(file, str); // comment line

    getline(file, str); // col indices line
    for (int row = 0; row < total_layers + 1; row++) {
        getline(file, str);
        for (int col = 0; col < total_layers; col++) {
            size_t start = str.find("(");
            size_t mid = str.find(",");
            size_t end = str.find(")");
            area_tables.emplace_back(str.substr(start + 1, mid - start - 1));
            fringe_tables.emplace_back(str.substr(mid + 2, end - mid - 2));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }
    }

    // useless lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    int total_area_tables = 0;
    for (int i = 1; i <= total_layers; i++)
        total_area_tables += i;
    int total_lateral_tables = total_layers;
    int total_fringe_tables = fringe_tables.size() - total_layers * 2;

    string table_name;
    for (int i = 0; i < total_area_tables; i++) {
        AreaTable atbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            atbl.s.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        while (start != string::npos) {
            atbl.a.emplace_back(atof((char *)str.substr(start + 1, mid - start - 1).c_str()));
            atbl.b.emplace_back(atof((char *)str.substr(mid + 2, end - mid - 2).c_str()));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
            start = str.find("(");
            mid = str.find(",");
            end = str.find(")");
        }

        getline(file, str); // empty line

        area_table_map[table_name] = atbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_lateral_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_fringe_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    file.close();
}

void ReadRule()
{
    ifstream file(path + rule_file);

    Rule r;
    string str;
    while (file >> r.layer >> str >> r.min_width >> r.min_space >> r.max_fill_width >> 
           r.min_density >> r.max_density)
        rules.emplace_back(r);

    file.close();
}

void AnalyzeDensity()
{
    stride = window_size / 2;
    long long w_area = window_size * window_size; 
    long long qw_area = stride * stride;
    // # windows
    window_x = cb.width_x / window_size * 2 - 1;
    window_y = cb.width_y / window_size * 2 - 1;
    // # quarter-windows
    qwindow_x = cb.width_x / stride;
    qwindow_y = cb.width_y / stride;

    int current_metal = 1;
    for (int layer = 1; layer <= total_layers; layer++) {
        // initialize quarter-window
        vector<QuarterWindow> qwindows(qwindow_x * qwindow_y);
        for (int x = 0; x < qwindow_x; x++) {
            int start = x * stride;
            int end = (x + 1) * stride;
            for (int y = 0; y < qwindow_y; y++) {
                int index = x * qwindow_y + y;
                qwindows[index].area = 0;
                qwindows[index].x_start = start;
                qwindows[index].x_end = end;
                qwindows[index].y_start = y * stride;
                qwindows[index].y_end = (y + 1) * stride;
                qwindows[index].violate_count = 0;
                qwindows[index].hasCritical = 0;
            }
        }

        // layout corrsponds to metal index
        Layout temp = layouts[current_metal];
        // index of metal in which quarter-window
        int x_from, x_to, y_from, y_to;
        // coordinate of partial metal in quarter window
        int x_start, x_end, y_start, y_end;
        long long area = 0;
        while (layer == temp.layer) {
		    x_from = temp.bl_x / stride;
		    x_to = (temp.tr_x - 1) / stride;
		    y_from = temp.bl_y / stride;
		    y_to = (temp.tr_y - 1) / stride;
		    for (int x = x_from; x <= x_to; x++) {
			    x_start = x == x_from ? temp.bl_x : x * stride;
			    x_end = x == x_to ? temp.tr_x : (x + 1) * stride;
			    for (int y = y_from; y <= y_to; y++) {
				    y_start = y == y_from ? temp.bl_y : y * stride;
				    y_end = y == y_to ? temp.tr_y : (y + 1) * stride;
				    area = (x_end - x_start) * (y_end - y_start);
				    qwindows[x * qwindow_y + y].area += area;
                    qwindows[x * qwindow_y + y].contribute_metals.emplace_back(temp.id);
                    if (temp.isCritical)
                        qwindows[x * qwindow_y + y].hasCritical++;
			    }
		    }
            if (++current_metal == total_metals + 1)
                break;
            temp = layouts[current_metal];
        }

        vector<Window> ws(window_x * window_y);
        for (int x = 0; x < window_x; x++) {
            for (int y = 0; y < window_y; y++) {
                int index = x * window_y + y;
                ws[index].area = qwindows[x * qwindow_y + y].area + qwindows[(x + 1) * qwindow_y + y].area +
                                 qwindows[x * qwindow_y + y + 1].area + qwindows[(x + 1) * qwindow_y + y + 1].area;
                ws[index].density = (double)ws[index].area / w_area;
                if (ws[index].density < rules[layer].min_density) {
                    qwindows[x * qwindow_y + y].violate_count++;
                    qwindows[(x + 1) * qwindow_y + y].violate_count++;
                    qwindows[x * qwindow_y + y + 1].violate_count++;
                    qwindows[(x + 1) * qwindow_y + y + 1].violate_count++; 
                }
            }
        }

        quarter_windows[layer - 1] = qwindows;
        windows[layer - 1] = ws;
    }
}

Rect FindSpaceSquare(const vector<int> &qw, int target_width, const int min_space)
{
    Rect rect = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x = -1, .width_y = -1};
    int temp_width = window_size;
    target_width += min_space * 2;

    int window_size_padding = window_size + 2;
    vector<int> area(window_size_padding * window_size_padding, 0);

    for (int y = 1; y <= window_size; y++) {
        for (int x = 1; x <= window_size; x++) {
            int idx = y * window_size_padding + x;
            if (!qw[(y - 1) * window_size + x - 1])
                area[idx] = min({area[idx - window_size_padding], area[idx - 1], area[idx - window_size_padding - 1]}) + 1;
            if (area[idx] >= target_width && area[idx] < temp_width) {
                rect.bl_x = x - area[idx]; // x - 1 - area[idx] + 1
                rect.bl_y = y - area[idx]; // y - 1 - area[idx] + 1
                rect.tr_x = x - 1;
                rect.tr_y = y - 1;
                temp_width = area[idx];
            }
        }
    }

    rect.bl_x += min_space;
    rect.bl_y += min_space;
    rect.tr_x -= min_space;
    rect.tr_y -= min_space;
    rect.width_x = rect.tr_x - rect.bl_x;
    rect.width_y = rect.tr_y - rect.bl_y;
    return rect;
}

// should find a space larger than target area, but the larger area depends on the space it found
Rect FindSpace(const vector<int> &qw, long long target_area, const int min_width, const int max_width, const int min_space)
{
    Rect rect = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x = -1, .width_y = -1};
    long long temp_area = window_size * window_size;
    int actual_min_width = min_width + 2 * min_space;

    vector<int> wl(window_size, 0);
    vector<int> wr(window_size, 0);
    vector<int> h(window_size, 0);
    vector<int> l(window_size, 0);
    vector<int> r(window_size, 0);

    for (int y = 0; y < window_size; y++) {
        // search how far it can extend on left
        for (int x = 0; x < window_size; x++) {
            if (!qw[y * window_size + x]) {
                if (x == 0)
                    wl[0] = 1;
                else
                    wl[x] = wl[x - 1] + 1;
            }
            else
                wl[x] = 0;
        }
        // search how far it can extend on right
        for (int x = window_size - 1; x >= 0; x--) {
            if (!qw[y * window_size + x]) {
                if (x == window_size - 1)
                    wr[window_size - 1] = 1;
                else
                    wr[x] = wr[x + 1] + 1;
            }
            else
                wr[x] = 0;
        }
        // search how far it can extend on top
        for (int x = 0; x < window_size; x++) {
            if (!qw[y * window_size + x])
                h[x]++;
            else
                h[x] = 0;
        }
        // search how far it can extend on left after reaching top
        for (int x = 0; x < window_size; x++) {
            if (l[x] == 0)
                l[x] = wl[x];
            else
                l[x] = min(l[x], wl[x]);
        }
        // search how far it can extend on right after reaching top
        for (int x = 0; x < window_size; x++) {
            if (r[x] == 0)
                r[x] = wr[x];
            else
                r[x] = min(r[x], wr[x]);
        }

        // search for the smallest matching area
        for (int x = 0; x < window_size; x++) {
            int width1 = l[x] + r[x] - 1;
            int width2 = h[x];
            if (width1 >= actual_min_width && width1 <= max_width && width2 >= actual_min_width && width2 <= max_width) {
                long long area = (width1 - 2 * min_space) * (width2 - 2 * min_space);
                if (area >= target_area && area < temp_area) {
                    rect.bl_x = x - l[x] + 1;
                    rect.bl_y = y - h[x] + 1;
                    rect.tr_x = x + r[x] - 1;
                    rect.tr_y = y;
                    temp_area = area;
                }
            }
        }
    }

    rect.bl_x += min_space;
    rect.bl_y += min_space;
    rect.tr_x -= min_space;
    rect.tr_y -= min_space;
    rect.width_x = rect.tr_x - rect.bl_x;
    rect.width_y = rect.tr_y - rect.bl_y;
    return rect;
}

void FillToWindow(vector<int> &qw, Rect rect)
{
    for (int y = rect.bl_y; y <= rect.tr_y; y++)
        for (int x = rect.bl_x; x <= rect.tr_x; x++)
            qw[y * window_size + x] = 1;
}

void FillMetalRandomly()
{
}

void free_memory()
{
    // int total_layers = metals.size();
    // for (int i = 0; i < total_layers; i++) {
    //     Metal *metal_x = metals[i];
    //     Metal *metal_y = metals[i];
    //     Metal *metal_x_next = NULL;
    //     Metal *metal_y_next = NULL;
    //     while (metal_x != NULL) {
    //         metal_x_next = metal_x->next_x;
    //         while (metal_y != NULL) {
    //             metal_y_next = metal_y->next_y;
    //             delete metal_y;
    //             metal_y = metal_y_next;
    //         }
    //         metal_x = metal_x_next;
    //         metal_y = metal_x;
    //     }
    //     metals[i] = NULL;
    // }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: ./<executable> <config file>\n");
        exit(1);
    }
    config_file = argv[1];
    ReadConfig();
    ReadCircuit();
    ReadProcess();
    ReadRule();

    windows.resize(total_layers);
    quarter_windows.resize(total_layers);
    AnalyzeDensity();
    
    // initialize cap table (symmetric lower traingular matrix)
    // long long total_map_size = total_metals * (total_metals + 1) / 2;
    // for (long long i = 0; i < total_map_size; i++)
    //     cap[i] = 0;
    // CalculateAreaCapacitance();
    // CalculateLateralCapacitance();
    // CalculateFringeCapacitance();

    FillMetalRandomly();

    // free_memory();

    return 0;
}
