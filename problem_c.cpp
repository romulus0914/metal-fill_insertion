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
    long long w_area = (long long)window_size * window_size;
    while (file >> r.layer >> str >> r.min_width >> r.min_space >> r.max_fill_width >> 
           r.min_density >> r.max_density) {
        min_area_per_window.emplace_back(w_area * r.min_density);
        rules.emplace_back(r);
    }

    file.close();
}

void AnalyzeDensity()
{
    stride = window_size / 2;
    long long w_area = (long long)window_size * window_size; 
    long long qw_area = (long long)stride * stride;
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
                qwindows[index].bl_x = start;
                qwindows[index].tr_x = end;
                qwindows[index].bl_y = y * stride;
                qwindows[index].tr_y = (y + 1) * stride;
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
        long long min_area = min_area_per_window[layer - 1];
        double min_density = rules[layer - 1].min_density;
        for (int x = 0; x < window_x; x++) {
            for (int y = 0; y < window_y; y++) {
                // window index
                int index = x * window_y + y;
                // quarter window index
                int q_index = x * qwindow_y + y;

                ws[index].index = index;
                ws[index].area = qwindows[q_index].area + qwindows[q_index + qwindow_y].area +
                                 qwindows[q_index + 1].area + qwindows[q_index + qwindow_y + 1].area;
                ws[index].area_insufficient = 0;
                ws[index].density = (double)ws[index].area / w_area;

                // remember which window will be affected
                qwindows[q_index].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y].affected_window.emplace_back(index);
                qwindows[q_index + 1].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y + 1].affected_window.emplace_back(index);

                if (ws[index].density < min_density) {
                    ws[index].area_insufficient = min_area - ws[index].area;
                    qwindows[q_index].violate_count++;
                    qwindows[q_index + qwindow_y].violate_count++;
                    qwindows[q_index + 1].violate_count++;
                    qwindows[q_index + qwindow_y + 1].violate_count++; 
                }
            }
        }

        quarter_windows[layer - 1] = qwindows;
        windows[layer - 1] = ws;
    }
}

// Rect FindSpaceSquare(const vector<int> &qw, const int target_width, const int min_space)
// {
//     Rect rt = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x = -1, .width_y = -1};
//     int temp_width = stride;

//     int stride_padding = stride + 2;
//     vector<int> area(stride_padding * stride_padding, 0);

//     for (int y = 1; y <= stride; y++) {
//         for (int x = 1; x <= stride; x++) {
//             int idx = y * stride_padding + x;
//             if (!qw[(y - 1) * stride + x - 1])
//                 area[idx] = min({area[idx - stride_padding], area[idx - 1], area[idx - stride_padding - 1]}) + 1;
//             if (area[idx] - 2 * min_space >= target_width && area[idx] < temp_width) {
//                 rt.bl_x = x - area[idx]; // x - 1 - area[idx] + 1
//                 rt.bl_y = y - area[idx]; // y - 1 - area[idx] + 1
//                 rt.tr_x = x - 1;
//                 rt.tr_y = y - 1;
//                 temp_width = area[idx];
//             }
//         }
//     }

//     rt.bl_x += min_space;
//     rt.bl_y += min_space;
//     rt.tr_x -= min_space;
//     rt.tr_y -= min_space;
//     rt.width_x = rt.tr_x - rt.bl_x;
//     rt.width_y = rt.tr_y - rt.bl_y;
//     return rt;
// }

// should find a space larger than target area, but the larger area it should be depends on the space it found
// if target_area == 0, find max space
Rect FindSpace(const vector<int> &qw, const long long target_area, const long long min_metal_fill,
               const int min_width, const int max_width, const int min_space)
{
    Rect rt = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x = -1, .width_y = -1};
    long long max_area = 0;
    long long temp_area = (long long)stride * stride;
    const int actual_min_width = min_width + 2 * min_space;
    const int actual_max_width = max_width + 2 * min_space;

    vector<int> wl(stride, 0);
    vector<int> wr(stride, 0);
    vector<int> h(stride, 0);
    vector<int> l(stride, 0);
    vector<int> r(stride, 0);

    for (int y = 0; y < stride; y++) {
        // search how far it can extend on left
        for (int x = 0; x < stride; x++) {
            if (!qw[y * stride + x]) {
                if (x == 0)
                    wl[0] = 1;
                else
                    wl[x] = wl[x - 1] + 1;
            }
            else
                wl[x] = 0;
        }
        // search how far it can extend on right
        for (int x = stride - 1; x >= 0; x--) {
            if (!qw[y * stride + x]) {
                if (x == stride - 1)
                    wr[stride - 1] = 1;
                else
                    wr[x] = wr[x + 1] + 1;
            }
            else
                wr[x] = 0;
        }
        // search how far it can extend on top
        for (int x = 0; x < stride; x++) {
            if (!qw[y * stride + x])
                h[x]++;
            else
                h[x] = 0;
        }
        // search how far it can extend on left after reaching top
        for (int x = 0; x < stride; x++) {
            if (l[x] == 0)
                l[x] = wl[x];
            else
                l[x] = min(l[x], wl[x]);
        }
        // search how far it can extend on right after reaching top
        for (int x = 0; x < stride; x++) {
            if (r[x] == 0)
                r[x] = wr[x];
            else
                r[x] = min(r[x], wr[x]);
        }

        // search for the smallest matching area
        for (int x = 0; x < stride; x++) {
            int width1 = l[x] + r[x] - 1;
            int width2 = h[x];
            // no constraint on max_width because empty space may always be larger than max_width
            if (width1 >= actual_min_width && width2 >= actual_min_width) {
                long long area = (long long)(width1 - 2 * min_space) * (width2 - 2 * min_space);
                if (target_area > 0) {
                    if (area >= target_area && area < temp_area) {
                        rt.bl_x = x - l[x] + 1 + min_space;
                        rt.bl_y = y - h[x] + 1 + min_space;
                        rt.tr_x = x + r[x] - 1 - min_space;
                        rt.tr_y = y - min_space;
                        temp_area = area;
                    }
                }
                else {
                    if (area > max_area) {
                        rt.bl_x = x - l[x] + 1 + min_space;
                        rt.bl_y = y - h[x] + 1 + min_space;
                        rt.tr_x = x + r[x] - 1 - min_space;
                        rt.tr_y = y - min_space;
                        max_area = area;
                    }
                }
            }
        }
    }

    rt.width_x = rt.tr_x - rt.bl_x;
    rt.width_y = rt.tr_y - rt.bl_y;
    // if target area smaller than min_width * width, than extand to it
    if (rt.bl_x != -1 && target_area != 0 && target_area < min_metal_fill) {
        rt.tr_x = rt.bl_x + min_width;
        rt.tr_y = rt.bl_y + min_width;
        rt.width_x = min_width;
        rt.width_y = min_width;
    }
    else {
        // if can only find width larger than max_width, than shrink to max_width
        if (rt.width_x > max_width) {
            rt.tr_x = rt.bl_x + max_width;
            rt.width_x = max_width;
        }
        if (rt.width_y > max_width) {
            rt.tr_y = rt.bl_y + max_width;
            rt.width_y = max_width;
        }
    }

    return rt;
}

void FillWindowByRect(vector<int> &qw_area, const Rect rt)
{
    for (int y = rt.bl_y; y <= rt.tr_y; y++)
        for (int x = rt.bl_x; x <= rt.tr_x; x++)
            qw_area[y * stride + x] = 1;
}

void FillWindowById(vector<int> &qw_area, const QuarterWindow &qw, const int id)
{
    const Layout &temp = layouts[id];
    int x_start = temp.bl_x < qw.bl_x ? 0 : temp.bl_x - qw.bl_x;
    int x_end = temp.tr_x > qw.tr_x ? stride : temp.tr_x - qw.bl_x;
    int y_start = temp.bl_y < qw.bl_y ? 0 : temp.bl_y - qw.bl_y;
    int y_end = temp.tr_y > qw.tr_y ? stride : temp.tr_y - qw.bl_y;

    for (int y = y_start; y < y_end; y++)
        for (int x = x_start; x < x_end; x++)
            qw_area[y * stride + x] = 1;
}

int AddMetalFill(const Rect rt, const int layer, const int x_offset, const  int y_offset)
{
    Layout metal_fill;
    metal_fill.id = ++total_metals;
    // chessboard coordinate to 2D coordinate (need to + 1?)
    metal_fill.bl_x = rt.bl_x + x_offset + 1;
    metal_fill.bl_y = rt.bl_y + y_offset + 1;
    metal_fill.tr_x = rt.tr_x + x_offset + 1;
    metal_fill.tr_y = rt.tr_y + y_offset + 1;
    metal_fill.net_id = 0;
    metal_fill.layer = layer;
    metal_fill.type = 3; // Fill
    metal_fill.isCritical = false;

    layouts.emplace_back(metal_fill);
    total_fills++;

    return metal_fill.id;
}

long long UpdateWindows(vector<Window> &ws, QuarterWindow &qw, const Rect rt, const int layer)
{
    int new_id = AddMetalFill(rt, layer, qw.bl_x, qw.bl_y);
    long long metal_fill_area = (long long)rt.width_x * rt.width_y;

    qw.area += metal_fill_area;
    qw.contribute_metals.emplace_back(new_id);

    // update affected windows
    for (int w_idx : qw.affected_window) {
        ws[w_idx].area += metal_fill_area;
        ws[w_idx].area_insufficient -= metal_fill_area;
    }

    return metal_fill_area;
}

void FillMetalRandomly()
{
    for (int layer = 1; layer <= total_layers; layer++) {
        vector<Window> &ws = windows[layer - 1];
        vector<QuarterWindow> &qws = quarter_windows[layer - 1];
        const Rule &r = rules[layer - 1];
        long long min_metal_fill = r.min_width * r.min_width;

        while(1) {
            // find the largest insufficient area window
            int max_insuff_idx = -1;
            long long max_insuff_area = 0;
            for (Window w : ws) {
                if (w.area_insufficient > max_insuff_area) {
                    max_insuff_idx = w.index;
                    max_insuff_area = w.area_insufficient;
                }
            }
            // all fulfill min density
            if (max_insuff_area == 0)
                break;

            // window x, y index for calculating quarter window x, y index
            int x_idx = max_insuff_idx / window_y;
            int y_idx = max_insuff_idx % window_y;
            // included quarter window
            vector<int> qw_idx{x_idx * qwindow_y + y_idx, x_idx * qwindow_y + y_idx + 1,
                            (x_idx + 1) * qwindow_y + y_idx, (x_idx + 1) * qwindow_y + y_idx + 1};
            // insertion sort by # included critical nets
            // start metal fill in quarter windows with less critical nets
            for (int i = 1; i < 4; i++) {
                int temp = qw_idx[i];
                int j = i - 1;
                while (j >= 0 && qws[qw_idx[j]].hasCritical > qws[temp].hasCritical) {
                    qw_idx[j + 1] = qw_idx[j];
                    j--;
                }
                qw_idx[j + 1] = temp;
            }

            // fill metal in sorted quarter window order
            for (int current_qw : qw_idx) {
                vector<int> qw_area(stride * stride, 0);
                QuarterWindow &qw = qws[current_qw];

                // occupied space by original metals
                for (int metal_id : qw.contribute_metals)
                    FillWindowById(qw_area, qw, metal_id);

                // try to fill with max space in quarter window
                // because of constraint in function, max space will be no larger than max_width * max_width
                long long target_area = 0;
                while (max_insuff_area > 0) {
                    Rect metal_fill = FindSpace(qw_area, target_area, min_metal_fill, 
                                                r.min_width, r.max_fill_width, r.min_space);
                    if (metal_fill.bl_x == -1)
                        break;

                    long long metal_fill_area = (long long)metal_fill.width_x * metal_fill.width_y;
                    if (metal_fill_area > max_insuff_area && target_area == 0)
                        target_area = max_insuff_area;
                    else {
                        FillWindowByRect(qw_area, metal_fill);
                        max_insuff_area -= UpdateWindows(ws, qw, metal_fill, layer);
                    }
                }

                // no more insufficient area to fill
                if (max_insuff_area <= 0)
                    break;
            }

            // error if after fill in four quarter windows, there is still insufficient area
            if (max_insuff_area > 0) {
                printf("[Error] error in dummy metal fill\n");
                exit(1);
            }
        }
    }
}

void OutputLayout()
{
    ofstream file(path + "circuit_metal-fill.cut");

    file << cb.bl_x << " " << cb.bl_y << " " << cb.tr_x << " " << cb.tr_y << "; chip boundary\n";

    for (int i = 1; i <= total_metals; i++) {
        Layout &temp = layouts[i];
        file << temp.id << " " << temp.bl_x + cb.bl_x << " " << temp.bl_y + cb.bl_y << " "
             << temp.tr_x + cb.bl_x << " " << temp.tr_y + cb.bl_y << " "
             << temp.net_id << " " << temp.layer << " ";
        if (temp.type == 0)
            file << "Drv_Pin\n";
        else if (temp.type == 1)
            file << "Normal\n";
        else if (temp.type == 2)
            file << "Load_Pin\n";
        else if (temp.type == 3)
            file << "Fill\n";
    }

    file.close();
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

    OutputLayout();

    // free_memory();

    return 0;
}
