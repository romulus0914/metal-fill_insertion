#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "problem_c.h"

using namespace std;

void read_config()
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

void read_circuit()
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

void read_process()
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
        Area_Table atbl;
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
        Fringe_Table ftbl;
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
        Fringe_Table ftbl;
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

void read_rule()
{
    ifstream file(path + rule_file);

    Rule r;
    string str;
    while (file >> r.layer >> str >> r.min_width >> r.min_space >> r.max_fill_width >> 
           r.min_density >> r.max_density)
        rules.emplace_back(r);

    file.close();
}

void analyze_density()
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
        vector<Quarter_Window> qwindows(qwindow_x * qwindow_y);
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
                ws[index].density = (double)ws[x * window_y + y].area / w_area;
                if (ws[index].density < rules[layer].min_density) {
                    qwindows[x * qwindow_y + y].violate_count++;
                    qwindows[(x + 1) * qwindow_y + y].violate_count++;
                    qwindows[x * qwindow_y + y + 1].violate_count++;
                    qwindows[(x + 1) * qwindow_y + y + 1].violate_count++;
                }
            }
        }
        quarter_windows.emplace_back(qwindows);
        windows.emplace_back(ws);
    }
}

/*
    type: ["area""lateral""fringe"]
    layer1: [0-9]
    layer2: [1-9]
    param1: parameter to look up in tables, (s for "area", d for "lateral" and "fringe")
    param2: parameter to multiply, (s for "area", l for "lateral" and "fringe")
*/
double calculate_coupling_capacitance(string type,int layer1, int layer2, double param1, double param2)
{
    if (type == "area") {
        const Area_Table &atbl = area_table_map[area_tables[layer1 * total_layers + layer2]];
        int size = atbl.s.size();
        for (int i = 0; i < size; i++) {
            if (param1 < atbl.s[i]) {
                if (i == 0)
                    return (atbl.a[0] * atbl.s[0] + atbl.b[0]) * atbl.s[0] * (param1 / atbl.s[0]);
                else
                    return (atbl.a[i - 1] * param1 + atbl.b[i - 1]) * param1;
            }
        }
        int last = size - 1;
        return (atbl.a[last] * atbl.s[last] + atbl.b[last]) * atbl.s[last] * (param1 / atbl.s[last]);
    }
    else if (type == "lateral") {
        const Fringe_Table &ltbl = fringe_table_map[fringe_tables[layer1 * total_layers + layer1]]; // layer1 == layer2
        int size = ltbl.d.size();
        for (int i = 0; i < size; i++) {
            if (param1 < ltbl.d[i]) {
                if (i == 0)
                    return 0.0;
                else
                    return (ltbl.a[i - 1] * param1 + ltbl.b[i - 1]) * param2;
            }
        }
        return 0.0;
    }
    else if (type == "fringe") {
        const Fringe_Table &ftbl1 = fringe_table_map[fringe_tables[layer1 * total_layers + layer2]];
        const Fringe_Table &ftbl2 = fringe_table_map[fringe_tables[layer2 * total_layers + layer1]];
        double coupling_cap = 0.0;
        int size = ftbl1.d.size();
        for (int i = 0; i < size; i++)
            if (i != 0 && param1 < ftbl1.d[i])
                coupling_cap = (ftbl1.a[i - 1] * param1 + ftbl1.b[i - 1]) * param2;
        size = ftbl2.d.size();
        for (int i = 0; i < size; i++)
            if (i != 0 && param1 < ftbl2.d[i])
                return coupling_cap + (ftbl2.a[i - 1] * param1 + ftbl2.b[i - 1]) * param2;
        return coupling_cap;
    }
}

// transform two ids to symmetric, lower triangular matrix index (0 based)
long long id_to_index(int id1, int id2)
{
    if (id1 == 0 && id2 == 0)
        return 0;
    else if (id1 <= id2)
        return (1 + id2) * id2 / 2 + id1;
    else
        return (1 + id1) * id1 / 2 + id2;
}

void calculate_area_capacitance()
{
}

int calculate_shielding_lateral_cap(vector<int> &edge, const Layout &cur_metal, const Layout &temp, int distance, int start, int end)
{
    int length = 0;
    for (int i = start; i < end; i++) {
        if (edge[i] == 0) {
            length++;
            edge[i] = 1;
        }
    }
    
    // lateral capacitance
    double lateral_cap = 0.0;
    if (length != 0)
        lateral_cap = calculate_coupling_capacitance("lateral", cur_metal.layer, temp.layer, distance, length);
    cap[id_to_index(cur_metal.id - 1, temp.id - 1)] = lateral_cap;

    return length;
}

void calculate_lateral_capacitance()
{
    for (int current_metal = 1; current_metal <= total_metals; current_metal++) {
        const Layout &cur_metal = layouts[current_metal];
        // quarter window indexn
        int x_from = cur_metal.bl_x / stride;
        int x_to = (cur_metal.tr_x - 1) / stride;
        int y_from = cur_metal.bl_y / stride;
        int y_to = (cur_metal.tr_y - 1) / stride;

        // candidate lateral metals
        set<int> candidate_metals;
        // search quarter window for candidate metals, only need to consider the same row or col
        // same row
        for (int x = 0; x < qwindow_x; x++) {
            for (int y = y_from; y <= y_to; y++) {
                const vector<int> &qwindow_metals = quarter_windows[cur_metal.layer - 1][x * qwindow_y + y].contribute_metals;
                for (int metal_id : qwindow_metals) {
                    // skip itself
                    if (metal_id == current_metal)
                        continue;
                    // calculate already then skip
                    if (cap[id_to_index(current_metal - 1, metal_id - 1)] != 0)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }
        // same col
        for (int y = 0; y < qwindow_y; y++) {
            for (int x = x_from; x <= x_to; x++) {
                const vector<int> &qwindow_metals = quarter_windows[cur_metal.layer - 1][x * qwindow_y + y].contribute_metals;
                for (int metal_id : qwindow_metals) {
                    // skip itself
                    if (metal_id == current_metal)
                        continue;
                    // calculate already then skip
                    if (cap[id_to_index(current_metal - 1, metal_id - 1)] != 0)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }

        // determine remaining metals belong to which direction
        vector<int> up, down, left, right;
        for (int metal_id : candidate_metals) {
            const Layout &temp = layouts[metal_id];
            // determine if they can't see each other then skip (haven't consider shielding)
            if ((temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x) &&
                (temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
                continue;
            // determine in which direction
            if (temp.bl_y >=  cur_metal.tr_y)
                up.emplace_back(metal_id);
            else if (temp.tr_y <= cur_metal.bl_y)
                down.emplace_back(metal_id);
            else if (temp.tr_x <= cur_metal.bl_x)
                left.emplace_back(metal_id);
            else if (temp.bl_x >= cur_metal.tr_x)
                right.emplace_back(metal_id);
            else {
                /* shouldn't overlap, testcase problems, via metal */
                continue;
                // printf("[Error] error in calculating lateral capcitance\n");
                // printf("[Error] error in determine in which set\n");
                // printf("[Error] %d:%d (%d, %d, %d, %d)\n", cur_metal.id, cur_metal.layer, cur_metal.bl_x, cur_metal.bl_y, cur_metal.tr_x, cur_metal.tr_y);
                // printf("[Error] %d:%d (%d, %d, %d, %d)\n", temp.id, temp.layer, temp.bl_x, temp.bl_y, temp.tr_x, temp.tr_y);
                // exit(1);
            }
        }
        set<int>().swap(candidate_metals); // not sure if this works for set as vector to free memory

        // the nearer to the cur_metal, the sooner the metal should be evaluate
        sort(up.begin(), up.end(), [](const int &i, const int &j) {
            return layouts[i].bl_y < layouts[j].bl_y;
        });
        sort(down.begin(), down.end(), [](const int &i, const int &j) {
            return layouts[i].tr_y > layouts[j].tr_y;
        });
        sort(left.begin(), left.end(), [](const int &i, const int &j) {
            return layouts[i].tr_x > layouts[j].tr_x;
        });
        sort(right.begin(), right.end(), [](const int &i, const int &j) {
            return layouts[i].bl_x < layouts[j].bl_x;
        });

        // up
        int width = cur_metal.tr_x - cur_metal.bl_x;
        vector<int> edge(width, 0);
        // remaining lateral edge
        int remaining_length = width;
        for (int metal_id : up) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            remaining_length -= calculate_shielding_lateral_cap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // down
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : down) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = cur_metal.bl_y - temp.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            calculate_shielding_lateral_cap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // left
        width = cur_metal.tr_y - cur_metal.bl_y;
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : left) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = cur_metal.bl_x - temp.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;
            int y_end = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;

            calculate_shielding_lateral_cap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // right
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : right) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_x - cur_metal.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;
            int y_end = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;

            calculate_shielding_lateral_cap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        vector<int>().swap(edge);
        //printf("%d\r", current_metal);
    }
}

int calculate_shielding_fringe_cap(vector<int> &edge, const Layout &cur_metal, const Layout &temp, int distance, int start, int end)
{
    // no fringe cap between metals that are in same layer or projection overlapping
    if ((cur_metal.layer == temp.layer) ||
        !(temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x || temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
        return 0;

    int length = 0;
    for (int i = start; i < end; i++) {
        if (edge[i] == 0) {
            length++;
            edge[i] = 1;
        }
    }
    
    // fringe capacitance
    double fringe_cap = 0.0;
    if (length != 0)
        fringe_cap = calculate_coupling_capacitance("fringe", cur_metal.layer, temp.layer, distance, length);
    cap[id_to_index(cur_metal.id - 1, temp.id - 1)] = fringe_cap;

    return length;
}

// should be similar to lateral, but consider overlapping coordinate in different layers
void calculate_fringe_capacitance()
{
    for (int current_metal = 1; current_metal <= total_metals; current_metal++) {
        const Layout &cur_metal = layouts[current_metal];
        // quarter window index
        int x_from = cur_metal.bl_x / stride;
        int x_to = (cur_metal.tr_x - 1) / stride;
        int y_from = cur_metal.bl_y / stride;
        int y_to = (cur_metal.tr_y - 1) / stride;

        // candidate fringe metals
        set<int> candidate_metals;
        for (int layer = cur_metal.layer; layer <= total_layers; layer++) {
            // search quarter window for candidate metals, only need to consider the same row or col non matter in which layer
            // same row
            for (int x = 0; x < qwindow_x; x++) {
                for (int y = y_from; y <= y_to; y++) {
                    const vector<int> &qwindow_metals = quarter_windows[layer - 1][x * qwindow_y + y].contribute_metals;
                    for (int metal_id : qwindow_metals) {
                        // skip itself
                        if (metal_id == current_metal)
                            continue;
                        /* because it search from cur_metal.layer to layer 9, no cap would calculate twice */
                        // calculate already then skip
                        // if (cap[id_to_index(current_metal - 1, metal_id - 1)] != 0)
                        //     continue;
                        candidate_metals.insert(metal_id);
                    }
                }
            }
            // same col
            for (int y = 0; y < qwindow_y; y++) {
                for (int x = x_from; x <= x_to; x++) {
                    const vector<int> &qwindow_metals = quarter_windows[layer - 1][x * qwindow_y + y].contribute_metals;
                    for (int metal_id : qwindow_metals) {
                        // skip itself
                        if (metal_id == current_metal)
                            continue;
                        /* because it search from cur_metal.layer to layer 9, no cap would calculate twice */
                        // calculate already then skip
                        // if (cap[id_to_index(current_metal - 1, metal_id - 1)] != 0)
                        //     continue;
                        candidate_metals.insert(metal_id);
                    }
                }
            }
        }

        // determine remaining metals belong to which direction
        vector<int> up, down, left, right;
        for (int metal_id : candidate_metals) {
             const Layout &temp = layouts[metal_id];
            // determine if they can't see each other then skip (haven't consider shielding)
            if ((temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x) &&
                (temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
                continue;
            // determine in which direction
            // unlike lateral, one metal may belong to more than one direction because projection ovelapping from different layers
            if (temp.tr_y > cur_metal.tr_y)
                up.emplace_back(metal_id);
            if (temp.bl_y < cur_metal.bl_y)
                down.emplace_back(metal_id);
            if (temp.bl_x < cur_metal.bl_x)
                left.emplace_back(metal_id);
            if (temp.tr_x > cur_metal.tr_x)
                right.emplace_back(metal_id);
        }
        set<int>().swap(candidate_metals); // not sure if this works for set as vector to free memory

        // the nearer to the cur_metal, the sooner the metal should be evaluate
        // consider the projection overlapping metals the same, their orders don't matter
        //     just make sure they are in front of other metals 
        sort(up.begin(), up.end(), [](const int &i, const int &j) {
            return layouts[i].bl_y < layouts[j].bl_y;
        });
        sort(down.begin(), down.end(), [](const int &i, const int &j) {
            return layouts[i].tr_y > layouts[j].tr_y;
        });
        sort(left.begin(), left.end(), [](const int &i, const int &j) {
            return layouts[i].tr_x > layouts[j].tr_x;
        });
        sort(right.begin(), right.end(), [](const int &i, const int &j) {
            return layouts[i].bl_x < layouts[j].bl_x;
        });
    }
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
    read_config();
    read_circuit();
    read_process();
    read_rule();

    analyze_density();
    
    long long total_map_size = total_metals * (total_metals + 1) / 2;
    for (long long i = 0; i < total_map_size; i++)
        cap[i] = 0;
    //calculate_area_capacitance();
    calculate_lateral_capacitance();
    //calculate_fringe_capacitance();

    free_memory();

    return 0;
}
