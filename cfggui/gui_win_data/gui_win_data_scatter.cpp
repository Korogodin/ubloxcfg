/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

#include <cstring>

#include "ff_trafo.h"

#include "gui_inc.hpp"

#include "gui_win_data_scatter.hpp"

/* ****************************************************************************************************************** */

GuiWinDataScatter::GuiWinDataScatter(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _showingErrorEll   { false },
    _triggerSnapRadius { false }
{
    _winSize    = { 75.0, 0.0 };
    _winSizeMin = { 40.0, 0.0 };
    _winFlags  |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    GuiSettings::GetValue(_winName + ".showStats",     _showStats,     false);
    GuiSettings::GetValue(_winName + ".plotRadius",    _plotRadius,    10.0);
    GuiSettings::GetValue(_winName + ".showHistogram", _showHistogram, false);
    GuiSettings::GetValue(_winName + ".showAccEst",    _showAccEst,    true);
    for (int ix = 0; ix < NUM_SIGMA; ix++)
    {
        bool ena;
        GuiSettings::GetValue(_winName + ".sigmaEnabled" + std::to_string(ix), ena, false);
        _sigmaEnabled[ix] = ena;
        if (ena)
        {
            _showingErrorEll = true;
        }
    }

    ClearData();
}

GuiWinDataScatter::~GuiWinDataScatter()
{
    GuiSettings::SetValue(_winName + ".showStats",     _showStats);
    GuiSettings::SetValue(_winName + ".plotRadius",    _plotRadius);
    GuiSettings::SetValue(_winName + ".showHistogram", _showHistogram);
    GuiSettings::SetValue(_winName + ".showAccEst",    _showAccEst);
    for (int ix = 0; ix < NUM_SIGMA; ix++)
    {
        GuiSettings::SetValue(_winName + ".sigmaEnabled" + std::to_string(ix), _sigmaEnabled[ix]);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataScatter::_ProcessData(const InputData &data)
{
    // New epoch means database stats have updated
    if (data.type == InputData::DATA_EPOCH)
    {
        _dbinfo = _database->GetInfo();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataScatter::_ClearData()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataScatter::_DrawToolbar()
{
    Gui::VerticalSeparator();

    switch (_database->GetRefPos())
    {
        case Database::REFPOS_MEAN:
            if (ImGui::Button(ICON_FK_DOT_CIRCLE_O "###RefPos", GuiSettings::iconSize))
            {
                _database->SetRefPosLast();
            }
            Gui::ItemTooltip("Reference position is mean position");
            break;
        case Database::REFPOS_LAST:
            if (ImGui::Button(ICON_FK_CHECK_CIRCLE_O "###RefPos", GuiSettings::iconSize))
            {
                _database->SetRefPosMean();
            }
            Gui::ItemTooltip("Reference position is last position");
            break;
        case Database::REFPOS_USER:
            if (ImGui::Button(ICON_FK_CIRCLE_O "###RefPos", GuiSettings::iconSize))
            {
                _database->SetRefPosMean();
            }
            Gui::ItemTooltip("Reference position is user position");
            break;
    }

    ImGui::SameLine();

    // Fit data
    if (ImGui::Button(ICON_FK_ARROWS_ALT "###Fit", GuiSettings::iconSize))
    {
        _triggerSnapRadius = true;
    }
    Gui::ItemTooltip("Fit all points");

    ImGui::SameLine();

    // Error ellipse
    if (Gui::ToggleButton(ICON_FK_PERCENT "###ErrEll", NULL, &_showingErrorEll, "Error ellipses", NULL, GuiSettings::iconSize))
    {
        ImGui::OpenPopup("ErrEllSigma");
    }
    if (ImGui::BeginPopup("ErrEllSigma"))
    {
        for (int ix = 0; ix < NUM_SIGMA; ix++)
        {
            ImGui::Checkbox(SIGMA_LABELS[ix], &_sigmaEnabled[ix]);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Show statistics?
    Gui::ToggleButton(ICON_FK_CROP "###ShowStats", NULL, &_showStats,
        "Showing statistics", "Not showing statistics", GuiSettings::iconSize);

    ImGui::SameLine();

    // Histogram
    Gui::ToggleButton(ICON_FK_BAR_CHART "##Histogram", NULL, &_showHistogram,
        "Showing histogram", "Not showing histogram", GuiSettings::iconSize);

    ImGui::SameLine();

    // Accuracy estimate circle
    Gui::ToggleButton(ICON_FK_CIRCLE "###AccEst", NULL, &_showAccEst,
        "Showing accuracy estimate (2d)", "Not showing accuracy estimate (2d)", GuiSettings::iconSize);
}

// ---------------------------------------------------------------------------------------------------------------------

// For normal one-dimensional data 1, 2 and 3 sigma are the famous 68, 95 and 99.7%
// For normal two-dimensional data 1, 2 and 3 sigma are 38.5%, 86.5%, 98.9%
// Other scale factors for normal two-dimensional data: 2.4477 = 95%, 3.0349 = 99%, 3.7169 = 99.9%
// See http://johnthemathguy.blogspot.com/2017/11/statistics-of-multi-dimensional-data.html,
//     https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0118537

const float GuiWinDataScatter::SIGMA_SCALES[NUM_SIGMA] =
{
    1.0,              2.0,              2.4477,         3.0,              3.0349,         3.7169
};

const char * const GuiWinDataScatter::SIGMA_LABELS[NUM_SIGMA] =
{
    "1.0    (38.5%)", "2.0    (86.5%)", "2.4477 (95%)", "3.0    (98.9%)", "3.0349 (99%)", "3.7169 (99.9%)"
};

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataScatter::_DrawContent()
{
    ImGui::BeginChild("##Plot", ImVec2(0.0f, 0.0f), false,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Canvas
    const FfVec2f offs = ImGui::GetCursorScreenPos();
    const FfVec2f size = ImGui::GetContentRegionAvail();
    const FfVec2f cent = ImVec2(offs.x + std::floor(size.x * 0.5), offs.y + std::floor(size.y * 0.5));
    const FfVec2f cursorOrigin = ImGui::GetCursorPos();
    const FfVec2f cursorMax = ImGui::GetWindowContentRegionMax();

    constexpr float minRadiusM = 0.02;
    constexpr float maxRadiusM = 10000.0;

    double refPosLlh[3];
    double refPosXyz[3];
    const auto refPos = _database->GetRefPos(refPosLlh, refPosXyz);

    const float lineHeight = ImGui::GetTextLineHeight();
    const float radiusM = _plotRadius;
    auto &io = ImGui::GetIO();

    // Draw scatter plot (draw list stuff is in screen coordinates)
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float radiusPx = (MAX(size.x, size.y) / 2) - (3 * GuiSettings::charSize.x);
    const float m2px = radiusM > FLT_EPSILON ? radiusPx / radiusM : 1.0;
    const float px2m = radiusPx > FLT_EPSILON ? radiusM / radiusPx : 1.0;
    const float top = offs.y;
    const float bot = offs.y + size.y;
    const float left = offs.x;
    const float right = offs.x + size.x;
    draw->PushClipRect(offs, offs + size);

    // Draw histogram (will lag one epoch, but we want to draw it below the points...)
    if (_showHistogram && (_histNumPoints > 0))
    {
        const float barSep = 2 * radiusPx / (float)NUM_HIST;
        const float barLen = 0.5 * radiusPx;
        const float norm = barLen / (float)_histNumPoints;
        const FfVec2f pos0x = cent + FfVec2f(-radiusPx, -radiusPx);
        const FfVec2f pos0y = cent + FfVec2f(-radiusPx, radiusPx - barSep);
        for (int ix = 0; ix <NUM_HIST; ix++)
        {
            {
                const float dx = ix * barSep;
                const float dy = 1.0 + ((float)_histogramE[ix] * norm);
                draw->AddRectFilled(pos0x + FfVec2f(dx, 0), pos0x + FfVec2f(dx + barSep, dy), GUI_COLOUR(PLOT_HISTOGRAM));
            }
            {
                const float dx = 1.0 + ((float)_histogramN[ix] * norm);
                const float dy = -ix * barSep;
                draw->AddRectFilled(pos0y + FfVec2f(0, dy), pos0y + FfVec2f(dx, dy + barSep), GUI_COLOUR(PLOT_HISTOGRAM));
            }
        }
    }

    // Draw points (and update histogram)
    std::memset(_histogramN, 0, sizeof(_histogramN));
    std::memset(_histogramE, 0, sizeof(_histogramE));
    _histNumPoints = 0;
    bool havePoints = false;

    _database->ProcRows([&](const Database::Row &row)
    {
        if (row.pos_avail)
        {
            const float east  = row.pos_enu_ref_east;
            const float north = row.pos_enu_ref_north;
            const float dx = std::floor( (east * m2px) + 0.5 );
            const float dy = -std::floor( (north * m2px) + 0.5 );
            draw->AddRectFilled(cent + ImVec2(dx - 2, dy - 2), cent + ImVec2(dx + 2, dy + 2), row.fix_colour);
            havePoints = true;
            _histNumPoints++;

            if (_showHistogram)
            {
                const int binE = (radiusM + east) / (2 * _plotRadius) * (float)NUM_HIST;
                _histogramE[ binE < 0 ? 0 : (binE > (NUM_HIST - 1) ? (NUM_HIST - 1) : binE)]++;
                const int binN = (radiusM + north) / (2 * _plotRadius) * (float)NUM_HIST;
                _histogramN[ binN < 0 ? 0 : (binN > (NUM_HIST - 1) ? (NUM_HIST - 1) : binN)]++;
            }
        }
        return true;
    });

    // Draw grid
    {
        draw->AddLine(ImVec2(offs.x, cent.y), ImVec2(offs.x + size.x, cent.y), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(ImVec2(cent.x, offs.y), ImVec2(cent.x, offs.y + size.y), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddCircle(cent, radiusPx,        GUI_COLOUR(PLOT_GRID_MAJOR), 0);
        draw->AddCircle(cent, radiusPx * 0.75, GUI_COLOUR(PLOT_GRID_MINOR), 0);
        draw->AddCircle(cent, radiusPx * 0.5,  GUI_COLOUR(PLOT_GRID_MAJOR), 0);
        draw->AddCircle(cent, radiusPx * 0.25, GUI_COLOUR(PLOT_GRID_MINOR), 0);
    }

    // Label grid
    const float pos[] = { -1.0, -0.5, 0.5, 1.0 };
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(PLOT_GRID_LABEL));
    for (int ix = 0; ix < NUMOF(pos); ix++)
    {
        char label[20];
        float labelOffs = -0.2;
        if (radiusM > 49.9)
        {
            std::snprintf(label, sizeof(label), "%+.0f", pos[ix] * radiusM);
        }
        else if (radiusM > 0.19)
        {
            std::snprintf(label, sizeof(label), "%+.1f", pos[ix] * radiusM);
            labelOffs = 1.5;
        }
        else
        {
            std::snprintf(label, sizeof(label), "%+.2f", pos[ix] * radiusM);
            labelOffs = 2.5;
        }

        //std::snprintf(label, sizeof(label), radiusM < 0.2 ? "%+.2f" : "%+.1f", pos[ix] * radiusM);
        const float xOffs = ((float)strlen(label) - labelOffs) * GuiSettings::charSize.x;
        // x axis
        ImVec2 cursor = cent + ImVec2((radiusPx * pos[ix]) - xOffs, -GuiSettings::charSize.y - 2);
        ImGui::SetCursorScreenPos(cursor);
        ImGui::TextUnformatted(label);
        // y axis
        cursor = cent + ImVec2(-xOffs, radiusPx * -pos[ix]);
        if (pos[ix] > 0)
        {
            cursor.y -= GuiSettings::charSize.y + 2;
        }
        else
        {
            cursor.y += 2;
        }
        ImGui::SetCursorScreenPos(cursor);
        ImGui::TextUnformatted(label);
    }
    ImGui::PopStyleColor();

    // Draw min/max/mean
    if (_showStats && havePoints)
    {
        const float minEastX = cent.x + std::floor((_dbinfo.stats.pos_enu_ref_east.min * m2px) + 0.5);
        const float maxEastX = cent.x + std::floor((_dbinfo.stats.pos_enu_ref_east.max * m2px) + 0.5);
        draw->AddLine(ImVec2(minEastX, top), ImVec2(minEastX, bot), GUI_COLOUR(PLOT_STATS_MINMAX));
        draw->AddLine(ImVec2(maxEastX, top), ImVec2(maxEastX, bot), GUI_COLOUR(PLOT_STATS_MINMAX));
        const float minNorthY = cent.y - std::floor((_dbinfo.stats.pos_enu_ref_north.min * m2px) + 0.5);
        const float maxNorthY = cent.y - std::floor((_dbinfo.stats.pos_enu_ref_north.max * m2px) + 0.5);
        draw->AddLine(ImVec2(left, minNorthY), ImVec2(right, minNorthY), GUI_COLOUR(PLOT_STATS_MINMAX));
        draw->AddLine(ImVec2(left, maxNorthY), ImVec2(right, maxNorthY), GUI_COLOUR(PLOT_STATS_MINMAX));
        const float meanEastX = cent.x + std::floor((_dbinfo.stats.pos_enu_ref_east.mean * m2px) + 0.5);
        const float meanNorthY = cent.y - std::floor((_dbinfo.stats.pos_enu_ref_north.mean * m2px) + 0.5);
        if (refPos != Database::REFPOS_MEAN)
        {
            draw->AddLine(ImVec2(meanEastX, top), ImVec2(meanEastX, bot), GUI_COLOUR(PLOT_STATS_MEAN));
            draw->AddLine(ImVec2(left, meanNorthY), ImVec2(right, meanNorthY), GUI_COLOUR(PLOT_STATS_MEAN));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(PLOT_STATS_MINMAX));
        char label[20];

        std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_east.min);
        ImGui::SetCursorScreenPos(ImVec2(minEastX - (GuiSettings::charSize.x * strlen(label)) - 2, top));
        ImGui::TextUnformatted(label);

        std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_east.max);
        ImGui::SetCursorScreenPos(ImVec2(maxEastX + 2, top));
        ImGui::TextUnformatted(label);

        std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_north.min);
        ImGui::SetCursorScreenPos(ImVec2(left, minNorthY));
        ImGui::TextUnformatted(label);

        std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_north.max);
        ImGui::SetCursorScreenPos(ImVec2(left, maxNorthY - lineHeight));
        ImGui::TextUnformatted(label);

        ImGui::PopStyleColor();

        // Show mean East-North in case centre of plot isn't the mean
        if (refPos != Database::REFPOS_MEAN)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(PLOT_STATS_MEAN));

            std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_east.mean);
            ImGui::SetCursorScreenPos(ImVec2(meanEastX + 2, top + lineHeight));
            ImGui::TextUnformatted(label);

            std::snprintf(label, sizeof(label), "%+.3f", _dbinfo.stats.pos_enu_ref_north.mean);
            ImGui::SetCursorScreenPos(ImVec2(left + 2 + (GuiSettings::charSize.x * 6), meanNorthY - lineHeight));
            ImGui::TextUnformatted(label);

            ImGui::PopStyleColor();
        }
    }

    // Draw error ellipses
    _showingErrorEll = false;
    if (!std::isnan(_dbinfo.err_ell.a))
    {
        for (int sIx = 0; sIx < NUM_SIGMA; sIx++)
        {
            if (!_sigmaEnabled[sIx])
            {
                continue;
            }
            _showingErrorEll = true;

            ImVec2 points[100];
            const double cosOmega = std::cos(_dbinfo.err_ell.omega);
            const double sinOmega = std::sin(_dbinfo.err_ell.omega);
            const double x0 = _dbinfo.stats.pos_enu_ref_east.mean * m2px;
            const double y0 = _dbinfo.stats.pos_enu_ref_north.mean * m2px;
            const double scale = SIGMA_SCALES[sIx];
            const double a = scale * _dbinfo.err_ell.a;
            const double b = scale * _dbinfo.err_ell.b;
            for (int ix = 0; ix < NUMOF(points); ix++)
            {
                const double t = (double)ix * (2 * M_PI / (double)NUMOF(points));
                const double cosT = std::cos(t);
                const double sinT = std::sin(t);
                const double dx = (a * cosT * cosOmega) - (b * sinT * sinOmega);
                const double dy = (a * cosT * sinOmega) - (b * sinT * cosOmega);
                points[ix].x = cent.x + std::floor(x0 + (dx * m2px) + 0.5);
                points[ix].y = cent.y - std::floor(y0 + (dy * m2px) + 0.5);
            }
            draw->AddPolyline(points, NUMOF(points), GUI_COLOUR(PLOT_ERR_ELL), true, 3.0f);
        }
    }

    // Highlight last point, draw accuracy estimate circle
    //double lastEnu[3] = { 0.0, 0.0, 0.0 };
    //double lastLlh[3] = { 0.0, 0.0, 0.0 };
    _database->ProcRows([&](const Database::Row &row)
    {
        if (!std::isnan(row.pos_enu_ref_east))
        {
            const float east  = row.pos_enu_ref_east;
            const float north = row.pos_enu_ref_north;
            const float dx = std::floor( (east * m2px) + 0.5 );
            const float dy = -std::floor( (north * m2px) + 0.5 );
            const ImU32 c = row.fix_ok ? GUI_COLOUR(PLOT_FIX_HL_OK) : GUI_COLOUR(PLOT_FIX_HL_MASKED);
            uint32_t age = TIME() - row.time_ts;
            const float w = age < 100 ? 3.0 : 1.0;
            const float l = 20.0; //age < 100 ? 40.0 : 20.0; //20 + ((100 - MIN(age, 100)) / 2);
            if (_showAccEst)
            {
                draw->AddCircleFilled(cent + ImVec2(dx, dy), (0.5 * row.pos_acc_horiz * m2px) + 0.5, GUI_COLOUR(PLOT_MAP_ACC_EST));
            }
            draw->AddLine(cent + ImVec2(dx - l, dy), cent + ImVec2(dx - 3, dy), c, w);
            draw->AddLine(cent + ImVec2(dx + 3, dy), cent + ImVec2(dx + l, dy), c, w);
            draw->AddLine(cent + ImVec2(dx, dy - l), cent + ImVec2(dx, dy - 3), c, w);
            draw->AddLine(cent + ImVec2(dx, dy + 3), cent + ImVec2(dx, dy + l), c, w);
            return false;
        }
        return true;
    }, true);

    draw->PopClipRect();

    // Place an invisible button on top of everything to capture mouse events (and disable windows moving)
    // Note the real buttons above must are placed first so that they will get the mouse events first (before this invisible button)
    ImGui::SetCursorPos(cursorOrigin);
    ImGui::InvisibleButton("canvas", size, ImGuiButtonFlags_MouseButtonLeft);
    const bool isHovered = ImGui::IsItemHovered();
    const bool isActive = ImGui::IsItemActive();

    // Dragging
    if (isActive && havePoints)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            std::memcpy(_refPosLlhDragStart, refPosLlh, sizeof(_refPosLlhDragStart));
            std::memcpy(_refPosXyzDragStart, refPosXyz, sizeof(_refPosLlhDragStart));
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            FfVec2f totalDrag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            if ( (totalDrag.x != 0.0) || (totalDrag.y != 0.0) )
            {
                FfVec2f totalDragM = totalDrag * px2m;
                double enu[3] = { -totalDragM.x, totalDragM.y, 0 };
                double newXyz[3];
                enu2xyz_vec(enu, _refPosXyzDragStart, _refPosLlhDragStart, newXyz);
                double newLlh[3];
                xyz2llh_vec(newXyz, newLlh);
                newLlh[2] = refPosLlh[2]; // keep height
                _database->SetRefPosLlh(newLlh);
            }
        }
    }

    // Crosshairs
    if (isHovered)
    {
        draw->AddLine(ImVec2(io.MousePos.x, top), ImVec2(io.MousePos.x, bot), GUI_COLOUR(PLOT_FIX_CROSSHAIRS));
        draw->AddLine(ImVec2(left, io.MousePos.y), ImVec2(right, io.MousePos.y), GUI_COLOUR(PLOT_FIX_CROSSHAIRS));

        const auto delta = (cent - io.MousePos) * (FfVec2f(-1.0f, 1.0f) * px2m);
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(PLOT_FIX_CROSSHAIRS_LABEL));
        char label[20];

        std::snprintf(label, sizeof(label), "%+.3f", delta.x);
        ImGui::SetCursorScreenPos(ImVec2(io.MousePos.x + 2, top + (_showStats ? (2 * lineHeight) : 0)));
        ImGui::TextUnformatted(label);

        std::snprintf(label, sizeof(label), "%+.3f", delta.y);
        ImGui::SetCursorScreenPos(ImVec2(left + 2 + (_showStats ? (GuiSettings::charSize.x * 13) : 0), io.MousePos.y - lineHeight));
        ImGui::TextUnformatted(label);

        ImGui::PopStyleColor();
    }

    // Print centre position
    ImGui::SetCursorPosY(cursorMax.y - lineHeight);
    if (havePoints)
    {
        ImGui::Text("%+.7f %+.7f %+.3f", rad2deg(refPosLlh[0]), rad2deg(refPosLlh[1]), refPosLlh[2]);
    }
    else
    {
        ImGui::TextUnformatted("no data");
    }

    if (_triggerSnapRadius)
    {
        _plotRadius = MIN(MAX(_dbinfo.stats.pos_enu_ref_east.max, _dbinfo.stats.pos_enu_ref_north.max), maxRadiusM);
    }

    // Range control using mouse wheel (but not while dragging)
    if ( (isHovered && !isActive) || _triggerSnapRadius)
    {
        if ( (io.MouseWheel != 0.0) || _triggerSnapRadius)
        {
            float delta = 0.0;
            // Zoom in
            if (io.MouseWheel > 0)
            {
                if      (_plotRadius <  0.3)   { delta =  -0.02; }
                else if (_plotRadius <  1.1)   { delta =  -0.2;  }
                else if (_plotRadius < 20.1)   { delta =  -1.0;  }
                else if (_plotRadius < 50.1)   { delta =  -5.0;  }
                else if (_plotRadius < 1000.1) { delta =  -10.0;  }
                else                           { delta = -100.0;  }
            }
            // Zoom out
            else
            {
                if      (_plotRadius > 999.9)  { delta = 100.0;  }
                else if (_plotRadius >  49.9)  { delta =  10.0;  }
                else if (_plotRadius >  19.9)  { delta =   5.0;  }
                else if (_plotRadius >   0.9)  { delta =   1.0;  }
                else if (_plotRadius >   0.19) { delta =   0.2;  }
                else                           { delta =   0.02; }
            }
            if (io.KeyCtrl)
            {
                delta *= 2.0;
            }
            if (_triggerSnapRadius)
            {
                _plotRadius = std::floor((_plotRadius + (1.0 * delta)) / delta) * delta;
                _triggerSnapRadius = false;
            }
            else
            {
                _plotRadius = std::floor((_plotRadius + (1.5 * delta)) / delta) * delta;
            }
            if (_plotRadius < minRadiusM)
            {
                _plotRadius = minRadiusM;
            }
            else if (_plotRadius > maxRadiusM)
            {
                _plotRadius = maxRadiusM;
            }
        }
        if (havePoints)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_None /*ImGuiMouseCursor_ResizeAll*/);
        }
    }

    ImGui::EndChild();
}

/* ****************************************************************************************************************** */
