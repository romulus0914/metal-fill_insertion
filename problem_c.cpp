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
                qwindows[index].index = index;
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
                // remember which quarter window is included
                ws[index].included_qwindow.emplace_back(q_index);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y);
                ws[index].included_qwindow.emplace_back(q_index + 1);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y + 1);

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

        quarter_windows.emplace_back(qwindows);
        windows.emplace_back(ws);
    }
}

Rect FindMaxSpace(const vector<int> &qw, const int min_width, const int max_width, const int min_space)
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

    rt.width_x = rt.tr_x - rt.bl_x;
    rt.width_y = rt.tr_y - rt.bl_y;

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

long long UpdateWindows(vector<int> &qw_area, vector<Window> &ws, QuarterWindow &qw, const Rect rt, const int layer)
{
    FillWindowByRect(qw_area, rt);
    int new_id = AddMetalFill(rt, layer, qw.bl_x, qw.bl_y);
    long long metal_fill_area = (long long)rt.width_x * rt.width_y;

    qw.area += metal_fill_area;
    qw.contribute_metals.emplace_back(new_id);

    // update affected windows
    for (int w_idx : qw.affected_window) {
        ws[w_idx].area += metal_fill_area;
        ws[w_idx].area_insufficient -= metal_fill_area;
        if (ws[w_idx].area_insufficient <= 0) {
            for (int qw_idx : ws[w_idx].included_qwindow)
                quarter_windows[layer - 1][qw_idx].violate_count--;
        }
    }

    return metal_fill_area;
}

long long ShrinkMetalFill(vector<int> &qw_area, vector<Window> &ws, QuarterWindow &qw, Rect rt,
                          long long target_area, const int min_width, const int layer)
{
    int x_width = rt.width_x;
    int y_width = rt.width_y;

    int offset = 10;
    int shrink_width;
    while (x_width >= min_width && y_width >= min_width && (long long)x_width * y_width > target_area) {
        // shrink at larger width
        shrink_width = x_width < y_width ? 1 : 0;
        x_width = shrink_width ? x_width : x_width - offset;
        y_width = shrink_width ? y_width - offset : y_width;
    }
    x_width = shrink_width ? x_width : x_width + offset;
    y_width = shrink_width ? y_width + offset : y_width;

    int x_offset = (rt.width_x - x_width) / 2;
    int y_offset = (rt.width_y - y_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = x_width;
    rt.width_y = y_width;
    printf(" %d*%d=%lld ", rt.width_x, rt.width_y, (long long)rt.width_x * rt.width_y);

    return UpdateWindows(qw_area, ws, qw, rt, layer);
}

long long DivideMetalFill(vector<int> &qw_area, vector<Window> &ws, QuarterWindow &qw, Rect rt, long long target_area,
                          const int min_width, const int max_width, const int min_space, const int layer)
{
    vector<Rect> rts;
    int x_width = rt.width_x;
    int y_width = rt.width_y;
    int max_width_padding = max_width + min_space;

    Rect r = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x =  max_width, .width_y =  max_width};
    int x_max_count = x_width / max_width_padding;
    int y_max_count = y_width / max_width_padding;
    // x, y = max width
    for (int y_count = 0; y_count < y_max_count; y_count++) {
        r.bl_y = rt.bl_y + y_count * max_width_padding;
        r.tr_y = r.bl_y + max_width;
        for (int x_count = 0; x_count < x_max_count; x_count++) {
            r.bl_x = rt.bl_x + x_count * max_width_padding;
            r.tr_x = r.bl_x + max_width;
            rts.emplace_back(r);
        }
    }

    int x_width_rest = x_width % max_width_padding;
    int y_width_rest = y_width % max_width_padding;
    // y = max width
    if (x_width_rest != 0 && x_width_rest >= min_width) {
        r.bl_x = rt.bl_x + x_max_count * max_width_padding;
        r.tr_x = r.bl_x + x_width_rest;
        r.width_x = x_width_rest;
        r.width_y = max_width;
        for (int y_count = 0; y_count < y_max_count; y_count++) {
            r.bl_y = rt.bl_y + y_count * max_width_padding;
            r.tr_y = r.bl_y + max_width;
            rts.emplace_back(r);
        }
    }
    // x = max width
    if (y_width_rest != 0 && y_width_rest >= min_width) {
        r.bl_y = rt.bl_y + y_max_count * max_width_padding;
        r.tr_y = r.bl_y + y_width_rest;
        r.width_y = y_width_rest;
        r.width_x = max_width;
        for (int x_count = 0; x_count < x_max_count; x_count++) {
            r.bl_x = rt.bl_x + x_count * max_width_padding;
            r.tr_x = r.bl_x + max_width;
            rts.emplace_back(r);
        }
    }

    // bottom right
    if (x_width_rest != 0 && x_width_rest >= min_width && y_width_rest != 0 && y_width_rest >= min_width) {
        r.bl_x = rt.bl_x + x_max_count * max_width_padding;
        r.tr_x = r.bl_x + x_width_rest;
        r.bl_y = rt.bl_y + y_max_count * max_width_padding;
        r.tr_y = r.bl_y + y_width_rest;
        r.width_x = x_width_rest;
        r.width_y = y_width_rest;
        rts.emplace_back(r);
    }

    // try to fill no larger than target area
    long long total_fill_area = 0;
    int size = rts.size();
    int i = 0;
    for (i = 0; i < size; i++) {
        long long rect_area = (long long)rts[i].width_x * rts[i].width_y;
        if (total_fill_area + rect_area > target_area)
            break;
        printf(" (%d, %d, %d, %d) ", rts[i].bl_x, rts[i].bl_y, rts[i].tr_x, rts[i].tr_y);
        total_fill_area += UpdateWindows(qw_area, ws, qw, rts[i], layer);
    }
    printf(" %lld ", total_fill_area);
    // if break, try to shrink
    if (i != size) {
        printf(" shrink:");
        total_fill_area += ShrinkMetalFill(qw_area, ws, qw, rts[i], target_area - total_fill_area, min_width, layer);
    }

    printf(" %lld ", total_fill_area);
    return total_fill_area;
}

long long MinMetalFill(vector<int> &qw_area, vector<Window> &ws, QuarterWindow &qw, Rect rt,
                       const int min_width, const int layer)
{
    int x_offset = (rt.width_x - min_width) / 2;
    int y_offset = (rt.width_y - min_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = min_width;
    rt.width_y = min_width;

    return UpdateWindows(qw_area, ws, qw, rt, layer);
}

void FillMetalRandomly()
{
    for (int layer = 1; layer <= total_layers; layer++) {
        vector<Window> &ws = windows[layer - 1];
        vector<QuarterWindow> &qws = quarter_windows[layer - 1];
        const Rule &r = rules[layer - 1];
        // max, min metal fill area
        long long min_metal_fill = (long long)r.min_width * r.min_width;
        long long max_metal_fill = (long long)r.max_fill_width * r.max_fill_width;

        while(1) {
            // find the most violate quarter qwindow
            int max_violate_idx = -1;
            int max_violate_count = 0;
            for (QuarterWindow qw : qws) {
                if (qw.violate_count > max_violate_count) {
                    max_violate_count = qw.violate_count;
                    max_violate_idx = qw.index;
                }
            }
            // no more violate
            if (max_violate_count == 0)
                break;

            printf("Max violate: %d(%d)\n", max_violate_idx, max_violate_count);
            // fill the window with min insufficient area first
            int min_insuff_idx = -1;
            long long min_insuff_area = (long long)window_size * window_size;
            for (int w_idx : qws[max_violate_idx].affected_window) {
                if (ws[w_idx].area_insufficient > 0 && ws[w_idx].area_insufficient < min_insuff_area) {
                    min_insuff_area = ws[w_idx].area_insufficient;
                    min_insuff_idx = w_idx;
                }
            }

            // sort by violate count
            // the previous found max_violate_idx is still guaranteed to be no smaller than indices at the front after sort
            vector<int> qw_included(ws[min_insuff_idx].included_qwindow);
            for (int i = 1; i < 4; i++) {
                int temp = qw_included[i];
                int j = i - 1;
                while (j >= 0 && qws[qw_included[j]].violate_count < qws[temp].violate_count) {
                    qw_included[j + 1] = qw_included[j];
                    j--;
                }
                qw_included[j + 1] = temp;
            }

            long long target_area = min_insuff_area;
            // metal fill in descending violate order
            for (int qw_idx : qw_included) {
                vector<int> qw_area(stride * stride, 0);
                QuarterWindow &qw = qws[qw_idx];

                // occupied space by original metals
                for (int metal_id : qw.contribute_metals)
                    FillWindowById(qw_area, qw, metal_id);

                // try to fill with max space in quarter window
                while (target_area > 0) {
                    Rect metal_fill = FindMaxSpace(qw_area, r.min_width, r.max_fill_width, r.min_space);
                    long long metal_fill_area = (long long)metal_fill.width_x * metal_fill.width_y;
                    printf("  %d %lld %lld\n", qw_idx, target_area, metal_fill_area);
                    // can't find max space occurs when no space larger than min_width * min_width
                    if (metal_fill.bl_x == -1)
                        break;

                    if (target_area < min_metal_fill) {
                        printf("    min\n");
                        target_area -= MinMetalFill(qw_area, ws, qw, metal_fill, r.min_width, layer);
                    }
                    else if (target_area <= max_metal_fill) {
                        if (target_area >= metal_fill_area) {
                            printf("    update\n");
                            target_area -= UpdateWindows(qw_area, ws, qw, metal_fill, layer);
                        }
                        else {
                            printf("    shrink: ");
                            target_area -= ShrinkMetalFill(qw_area, ws, qw, metal_fill, target_area, r.min_width, layer);
                            printf("\n");
                        }
                    }
                    else {
                        if (metal_fill_area <= max_metal_fill) {
                            printf("    update\n");
                            target_area -= UpdateWindows(qw_area, ws, qw, metal_fill, layer);
                        }
                        else {
                            printf("    divide: ");
                            target_area -= DivideMetalFill(qw_area, ws, qw, metal_fill, target_area,
                                                           r.min_width, r.max_fill_width, r.min_space, layer);
                            printf("\n");
                        }
                    }
                }

                // no more target area to fill
                if (target_area <= 0)
                    break;
            }
            
            // error if remain target area after metal fill
            if (target_area > 0) {
                printf("[Error] target arae remaining after metal fill\n");
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
