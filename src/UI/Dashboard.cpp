/*
 * Copyright (c) 2020-2021 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <IO/Manager.h>
#include <IO/Console.h>
#include <CSV/Player.h>
#include <JSON/Generator.h>
#include <Misc/TimerEvents.h>

#include "Dashboard.h"

using namespace UI;

/*
 * Pointer to the only instance of the class.
 */
static Dashboard *INSTANCE = nullptr;

//--------------------------------------------------------------------------------------------------
// Constructor/deconstructor & singleton
//--------------------------------------------------------------------------------------------------

/**
 * Constructor of the class.
 */
Dashboard::Dashboard()
    : m_latestJsonFrame(JFI_Empty())
{
    auto cp = CSV::Player::getInstance();
    auto io = IO::Manager::getInstance();
    auto ge = JSON::Generator::getInstance();
    auto te = Misc::TimerEvents::getInstance();
    connect(cp, SIGNAL(openChanged()), this, SLOT(resetData()));
    connect(te, SIGNAL(highFreqTimeout()), this, SLOT(updateData()));
    connect(io, SIGNAL(connectedChanged()), this, SLOT(resetData()));
    connect(ge, SIGNAL(jsonFileMapChanged()), this, SLOT(resetData()));
    connect(ge, &JSON::Generator::jsonChanged, this, &Dashboard::selectLatestJSON);
}

/**
 * Returns a pointer to the only instance of the class.
 */
Dashboard *Dashboard::getInstance()
{
    if (!INSTANCE)
        INSTANCE = new Dashboard();

    return INSTANCE;
}

//--------------------------------------------------------------------------------------------------
// Group/Dataset access functions
//--------------------------------------------------------------------------------------------------

QFont Dashboard::monoFont() const
{
    return QFont("Roboto Mono");
}

JSON::Group *Dashboard::getGroup(const int index)
{
    if (index < m_latestFrame.groupCount())
        return m_latestFrame.groups().at(index);

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
// Misc member access functions
//--------------------------------------------------------------------------------------------------

/**
 * Returns the title of the current JSON project/frame.
 */
QString Dashboard::title()
{
    return m_latestFrame.title();
}

/**
 * Returns @c true if there is any data available to generate the QML dashboard.
 */
bool Dashboard::available()
{
    return totalWidgetCount() > 0;
}

/**
 * Returns @c true if the current JSON frame is valid and ready-to-use by the QML
 * interface.
 */
bool Dashboard::frameValid() const
{
    return m_latestFrame.isValid();
}

//--------------------------------------------------------------------------------------------------
// Widget count functions
//--------------------------------------------------------------------------------------------------

/**
 * Returns the total number of widgets that compose the current JSON frame.
 * This function is used as a helper to the functions that use the global-index widget
 * system.
 *
 * In the case of this function, we do not care about the order of the items that generate
 * the summatory count of all widgets. But in the other global-index functions we need to
 * be careful to sincronize the order of the widgets in order to allow the global-index
 * system to work correctly.
 */
int Dashboard::totalWidgetCount() const
{
    // clang-format off
    const int count = mapCount() +
                      barCount() +
                      plotCount() +
                      gaugeCount() +
                      groupCount() +
                      compassCount() +
                      gyroscopeCount() +
                      thermometerCount() +
                      accelerometerCount();
    // clang-format on

    return count;
}

// clang-format off
int Dashboard::mapCount() const           { return m_mapWidgets.count();           }
int Dashboard::barCount() const           { return m_barWidgets.count();           }
int Dashboard::plotCount() const          { return m_plotWidgets.count();          }
int Dashboard::gaugeCount() const         { return m_gaugeWidgets.count();         }
int Dashboard::groupCount() const         { return m_latestFrame.groupCount();     }
int Dashboard::compassCount() const       { return m_compassWidgets.count();       }
int Dashboard::gyroscopeCount() const     { return m_gyroscopeWidgets.count();     }
int Dashboard::thermometerCount() const   { return m_thermometerWidgets.count();   }
int Dashboard::accelerometerCount() const { return m_accelerometerWidgets.count(); }
// clang-format on

//--------------------------------------------------------------------------------------------------
// Relative-to-global widget index utility functions
//--------------------------------------------------------------------------------------------------

/**
 * Returns a @c QStringList with the titles of all the widgets that compose the current
 * JSON frame.
 *
 * We need to be careful to sincronize the order of the widgets in order to allow
 * the global-index system to work correctly.
 */
QStringList Dashboard::widgetTitles() const
{
    // Warning: maintain same order as the view option repeaters in ViewOptions.qml!

    // clang-format off
    return groupTitles() +
           plotTitles() +
           barTitles() +
           gaugeTitles() +
           thermometerTitles() +
           compassTitles() +
           gyroscopeTitles() +
           accelerometerTitles() +
           mapTitles();
    // clang-format on
}

/**
 * Returns the widget-specific index for the widget with the specified @a index.
 *
 * To simplify operations and user interface generation in QML, this class represents
 * the widgets in two manners:
 *
 * - A global list of all widgets
 * - A widget-specific list for each type of widget
 *
 * The global index allows us to use the @c WidgetLoader class to load any type of
 * widget, which reduces the need of implementing QML-specific code for each widget
 * that Serial Studio implements.
 *
 * The relative index is used by the visibility switches in the QML user interface.
 *
 * We need to be careful to sincronize the order of the widgets in order to allow
 * the global-index system to work correctly.
 */
int Dashboard::relativeIndex(const int globalIndex) const
{
    //
    // Warning: relative widget index should be calculated using the same order as defined
    // by the UI::Dashboard::widgetTitles() function.
    //

    // Check if we should return group widget
    int index = globalIndex;
    if (index < groupCount())
        return index;

    // Check if we should return plot widget
    index -= groupCount();
    if (index < plotCount())
        return index;

    // Check if we should return bar widget
    index -= plotCount();
    if (index < barCount())
        return index;

    // Check if we should return gauge widget
    index -= barCount();
    if (index < gaugeCount())
        return index;

    // Check if we should return thermometer widget
    index -= gaugeCount();
    if (index < thermometerCount())
        return index;

    // Check if we should return compass widget
    index -= thermometerCount();
    if (index < compassCount())
        return index;

    // Check if we should return gyro widget
    index -= compassCount();
    if (index < gyroscopeCount())
        return index;

    // Check if we should return accelerometer widget
    index -= gyroscopeCount();
    if (index < accelerometerCount())
        return index;

    // Check if we should return map widget
    index -= accelerometerCount();
    if (index < mapCount())
        return index;

    // Return unknown widget
    return -1;
}

/**
 * Returns @c true if the widget with the specifed @a globalIndex should be
 * displayed in the QML user interface.
 *
 * To simplify operations and user interface generation in QML, this class represents
 * the widgets in two manners:
 *
 * - A global list of all widgets
 * - A widget-specific list for each type of widget
 *
 * The global index allows us to use the @c WidgetLoader class to load any type of
 * widget, which reduces the need of implementing QML-specific code for each widget
 * that Serial Studio implements.
 *
 * We need to be careful to sincronize the order of the widgets in order to allow
 * the global-index system to work correctly.
 */
bool Dashboard::widgetVisible(const int globalIndex) const
{
    bool visible = false;
    auto index = relativeIndex(globalIndex);

    switch (widgetType(globalIndex))
    {
        case WidgetType::Group:
            visible = groupVisible(index);
            break;
        case WidgetType::Plot:
            visible = plotVisible(index);
            break;
        case WidgetType::Bar:
            visible = barVisible(index);
            break;
        case WidgetType::Gauge:
            visible = gaugeVisible(index);
            break;
        case WidgetType::Thermometer:
            visible = thermometerVisible(index);
            break;
        case WidgetType::Compass:
            visible = compassVisible(index);
            break;
        case WidgetType::Gyroscope:
            visible = gyroscopeVisible(index);
            break;
        case WidgetType::Accelerometer:
            visible = accelerometerVisible(index);
            break;
        case WidgetType::Map:
            visible = mapVisible(index);
            break;
        default:
            visible = false;
            break;
    }

    return visible;
}

/**
 * Returns the appropiate SVG-icon to load for the widget at the specified @a globalIndex.
 *
 * To simplify operations and user interface generation in QML, this class represents
 * the widgets in two manners:
 *
 * - A global list of all widgets
 * - A widget-specific list for each type of widget
 *
 * The global index allows us to use the @c WidgetLoader class to load any type of
 * widget, which reduces the need of implementing QML-specific code for each widget
 * that Serial Studio implements.
 *
 * We need to be careful to sincronize the order of the widgets in order to allow
 * the global-index system to work correctly.
 */
QString Dashboard::widgetIcon(const int globalIndex) const
{
    switch (widgetType(globalIndex))
    {
        case WidgetType::Group:
            return "qrc:/icons/group.svg";
            break;
        case WidgetType::Plot:
            return "qrc:/icons/plot.svg";
            break;
        case WidgetType::Bar:
            return "qrc:/icons/bar.svg";
            break;
        case WidgetType::Gauge:
            return "qrc:/icons/gauge.svg";
            break;
        case WidgetType::Thermometer:
            return "qrc:/icons/thermometer.svg";
            break;
        case WidgetType::Compass:
            return "qrc:/icons/compass.svg";
            break;
        case WidgetType::Gyroscope:
            return "qrc:/icons/gyroscope.svg";
            break;
        case WidgetType::Accelerometer:
            return "qrc:/icons/accelerometer.svg";
            break;
        case WidgetType::Map:
            return "qrc:/icons/map.svg";
            break;
        default:
            return "qrc:/icons/close.svg";
            break;
    }
}

/**
 * Returns the type of widget that corresponds to the given @a globalIndex.
 *
 * Possible return values are:
 * - @c WidgetType::Unknown
 * - @c WidgetType::Group
 * - @c WidgetType::Plot
 * - @c WidgetType::Bar
 * - @c WidgetType::Gauge
 * - @c WidgetType::Thermometer
 * - @c WidgetType::Compass
 * - @c WidgetType::Gyroscope
 * - @c WidgetType::Accelerometer
 * - @c WidgetType::Map
 *
 * To simplify operations and user interface generation in QML, this class represents
 * the widgets in two manners:
 *
 * - A global list of all widgets
 * - A widget-specific list for each type of widget
 *
 * The global index allows us to use the @c WidgetLoader class to load any type of
 * widget, which reduces the need of implementing QML-specific code for each widget
 * that Serial Studio implements.
 *
 * We need to be careful to sincronize the order of the widgets in order to allow
 * the global-index system to work correctly.
 */
Dashboard::WidgetType Dashboard::widgetType(const int globalIndex) const
{
    //
    // Warning: relative widget index should be calculated using the same order as defined
    // by the UI::Dashboard::widgetTitles() function.
    //

    // Unitialized widget loader class
    if (globalIndex < 0)
        return WidgetType::Unknown;

    // Check if we should return group widget
    int index = globalIndex;
    if (index < groupCount())
        return WidgetType::Group;

    // Check if we should return plot widget
    index -= groupCount();
    if (index < plotCount())
        return WidgetType::Plot;

    // Check if we should return bar widget
    index -= plotCount();
    if (index < barCount())
        return WidgetType::Bar;

    // Check if we should return gauge widget
    index -= barCount();
    if (index < gaugeCount())
        return WidgetType::Gauge;

    // Check if we should return thermometer widget
    index -= gaugeCount();
    if (index < thermometerCount())
        return WidgetType::Thermometer;

    // Check if we should return compass widget
    index -= thermometerCount();
    if (index < compassCount())
        return WidgetType::Compass;

    // Check if we should return gyro widget
    index -= compassCount();
    if (index < gyroscopeCount())
        return WidgetType::Gyroscope;

    // Check if we should return accelerometer widget
    index -= gyroscopeCount();
    if (index < accelerometerCount())
        return WidgetType::Accelerometer;

    // Check if we should return map widget
    index -= accelerometerCount();
    if (index < mapCount())
        return WidgetType::Map;

    // Return unknown widget
    return WidgetType::Unknown;
}

//--------------------------------------------------------------------------------------------------
// Widget visibility access functions
//--------------------------------------------------------------------------------------------------

// clang-format off
bool Dashboard::barVisible(const int index) const           { return getVisibility(m_barVisibility, index);           }
bool Dashboard::mapVisible(const int index) const           { return getVisibility(m_mapVisibility, index);           }
bool Dashboard::plotVisible(const int index) const          { return getVisibility(m_plotVisibility, index);          }
bool Dashboard::groupVisible(const int index) const         { return getVisibility(m_groupVisibility, index);         }
bool Dashboard::gaugeVisible(const int index) const         { return getVisibility(m_gaugeVisibility, index);         }
bool Dashboard::compassVisible(const int index) const       { return getVisibility(m_compassVisibility, index);       }
bool Dashboard::gyroscopeVisible(const int index) const     { return getVisibility(m_gyroscopeVisibility, index);     }
bool Dashboard::thermometerVisible(const int index) const   { return getVisibility(m_thermometerVisibility, index);   }
bool Dashboard::accelerometerVisible(const int index) const { return getVisibility(m_accelerometerVisibility, index); }
// clang-format on

//--------------------------------------------------------------------------------------------------
// Widget title functions
//--------------------------------------------------------------------------------------------------

// clang-format off
QStringList Dashboard::barTitles() const           { return datasetTitles(m_barWidgets);         }
QStringList Dashboard::mapTitles() const           { return groupTitles(m_mapWidgets);           }
QStringList Dashboard::plotTitles() const          { return datasetTitles(m_plotWidgets);        }
QStringList Dashboard::groupTitles() const         { return groupTitles(m_latestFrame.groups()); }
QStringList Dashboard::gaugeTitles() const         { return datasetTitles(m_gaugeWidgets);       }
QStringList Dashboard::compassTitles() const       { return datasetTitles(m_compassWidgets);     }
QStringList Dashboard::gyroscopeTitles() const     { return groupTitles(m_gyroscopeWidgets);     }
QStringList Dashboard::thermometerTitles() const   { return datasetTitles(m_thermometerWidgets); }
QStringList Dashboard::accelerometerTitles() const { return groupTitles(m_accelerometerWidgets); }
// clang-format on

//--------------------------------------------------------------------------------------------------
// Visibility-related slots
//--------------------------------------------------------------------------------------------------

// clang-format off
void Dashboard::setBarVisible(const int i, const bool v)           { setVisibility(m_barVisibility, i, v);           }
void Dashboard::setMapVisible(const int i, const bool v)           { setVisibility(m_mapVisibility, i, v);           }
void Dashboard::setPlotVisible(const int i, const bool v)          { setVisibility(m_plotVisibility, i, v);          }
void Dashboard::setGroupVisible(const int i, const bool v)         { setVisibility(m_groupVisibility, i, v);         }
void Dashboard::setGaugeVisible(const int i, const bool v)         { setVisibility(m_gaugeVisibility, i, v);         }
void Dashboard::setCompassVisible(const int i, const bool v)       { setVisibility(m_compassVisibility, i, v);       }
void Dashboard::setGyroscopeVisible(const int i, const bool v)     { setVisibility(m_gyroscopeVisibility, i, v);     }
void Dashboard::setThermometerVisible(const int i, const bool v)   { setVisibility(m_thermometerVisibility, i, v);   }
void Dashboard::setAccelerometerVisible(const int i, const bool v) { setVisibility(m_accelerometerVisibility, i, v); }
// clang-format on

//--------------------------------------------------------------------------------------------------
// Frame data handling slots
//--------------------------------------------------------------------------------------------------

/**
 * Removes all available data from the UI when the device is disconnected or the CSV
 * file is closed.
 */
void Dashboard::resetData()
{
    // Make latest frame invalid
    m_latestJsonFrame = JFI_Empty();
    m_latestFrame.read(m_latestJsonFrame.jsonDocument.object());

    // Clear widget data
    m_barWidgets.clear();
    m_mapWidgets.clear();
    m_plotWidgets.clear();
    m_gaugeWidgets.clear();
    m_compassWidgets.clear();
    m_gyroscopeWidgets.clear();
    m_thermometerWidgets.clear();
    m_accelerometerWidgets.clear();

    // Clear widget visibility data
    m_barVisibility.clear();
    m_mapVisibility.clear();
    m_plotVisibility.clear();
    m_gaugeVisibility.clear();
    m_groupVisibility.clear();
    m_compassVisibility.clear();
    m_gyroscopeVisibility.clear();
    m_thermometerVisibility.clear();
    m_accelerometerVisibility.clear();

    // Update UI
    emit updated();
    emit dataReset();
    emit titleChanged();
    emit widgetCountChanged();
    emit widgetVisibilityChanged();
}

/**
 * Interprets the most recent JSON frame & signals the UI to regenerate itself.
 */
void Dashboard::updateData()
{
    // Save widget count
    int barC = barCount();
    int mapC = mapCount();
    int plotC = plotCount();
    int groupC = groupCount();
    int gaugeC = gaugeCount();
    int compassC = compassCount();
    int gyroscopeC = gyroscopeCount();
    int thermometerC = thermometerCount();
    int accelerometerC = accelerometerCount();

    // Save previous title
    auto pTitle = title();

    // Clear widget data
    m_barWidgets.clear();
    m_mapWidgets.clear();
    m_plotWidgets.clear();
    m_gaugeWidgets.clear();
    m_compassWidgets.clear();
    m_gyroscopeWidgets.clear();
    m_thermometerWidgets.clear();
    m_accelerometerWidgets.clear();

    // Check if frame is valid
    if (!m_latestFrame.read(m_latestJsonFrame.jsonDocument.object()))
        return;

    // Update widget vectors
    m_plotWidgets = getPlotWidgets();
    m_mapWidgets = getWidgetGroups("map");
    m_barWidgets = getWidgetDatasets("bar");
    m_gaugeWidgets = getWidgetDatasets("gauge");
    m_gyroscopeWidgets = getWidgetGroups("gyro");
    m_compassWidgets = getWidgetDatasets("compass");
    m_thermometerWidgets = getWidgetDatasets("thermometer");
    m_accelerometerWidgets = getWidgetGroups("accelerometer");

    // Check if we need to update title
    if (pTitle != title())
        emit titleChanged();

    // Check if we need to regenerate widgets
    bool regenerateWidgets = false;
    regenerateWidgets |= (barC != barCount());
    regenerateWidgets |= (mapC != mapCount());
    regenerateWidgets |= (plotC != plotCount());
    regenerateWidgets |= (gaugeC != gaugeCount());
    regenerateWidgets |= (groupC != groupCount());
    regenerateWidgets |= (compassC != compassCount());
    regenerateWidgets |= (gyroscopeC != gyroscopeCount());
    regenerateWidgets |= (thermometerC != thermometerCount());
    regenerateWidgets |= (accelerometerC != accelerometerCount());

    // Regenerate widget visiblity models
    if (regenerateWidgets)
    {
        m_barVisibility.clear();
        m_mapVisibility.clear();
        m_plotVisibility.clear();
        m_gaugeVisibility.clear();
        m_groupVisibility.clear();
        m_compassVisibility.clear();
        m_gyroscopeVisibility.clear();
        m_thermometerVisibility.clear();
        m_accelerometerVisibility.clear();

        int i;
        for (i = 0; i < barCount(); ++i)
            m_barVisibility.append(true);
        for (i = 0; i < mapCount(); ++i)
            m_mapVisibility.append(true);
        for (i = 0; i < plotCount(); ++i)
            m_plotVisibility.append(true);
        for (i = 0; i < gaugeCount(); ++i)
            m_gaugeVisibility.append(true);
        for (i = 0; i < groupCount(); ++i)
            m_groupVisibility.append(true);
        for (i = 0; i < compassCount(); ++i)
            m_compassVisibility.append(true);
        for (i = 0; i < gyroscopeCount(); ++i)
            m_gyroscopeVisibility.append(true);
        for (i = 0; i < thermometerCount(); ++i)
            m_thermometerVisibility.append(true);
        for (i = 0; i < accelerometerCount(); ++i)
            m_accelerometerVisibility.append(true);

        emit widgetCountChanged();
        emit widgetVisibilityChanged();
    }

    // Update UI
    emit updated();
}

/**
 * Ensures that only the most recent JSON document will be displayed on the user
 * interface.
 */
void Dashboard::selectLatestJSON(const JFI_Object &frameInfo)
{
    auto frameCount = frameInfo.frameNumber;
    auto currFrameCount = m_latestJsonFrame.frameNumber;

    if (currFrameCount < frameCount)
        m_latestJsonFrame = frameInfo;
}

//--------------------------------------------------------------------------------------------------
// Widget utility functions
//--------------------------------------------------------------------------------------------------

/**
 * Returns a vector with all the datasets that need to be plotted.
 */
QVector<JSON::Dataset *> Dashboard::getPlotWidgets() const
{
    QVector<JSON::Dataset *> widgets;
    foreach (auto group, m_latestFrame.groups())
    {
        foreach (auto dataset, group->datasets())
        {
            if (dataset->graph())
                widgets.append(dataset);
        }
    }

    return widgets;
}

/**
 * Returns a vector with all the groups that implement the widget with the specied
 * @a handle.
 */
QVector<JSON::Group *> Dashboard::getWidgetGroups(const QString &handle) const
{
    QVector<JSON::Group *> widgets;
    foreach (auto group, m_latestFrame.groups())
    {
        if (group->widget().toLower() == handle)
            widgets.append(group);
    }

    return widgets;
}

/**
 * Returns a vector with all the datasets that implement a widget with the specified
 * @a handle.
 */
QVector<JSON::Dataset *> Dashboard::getWidgetDatasets(const QString &handle) const
{
    QVector<JSON::Dataset *> widgets;
    foreach (auto group, m_latestFrame.groups())
    {
        foreach (auto dataset, group->datasets())
        {
            if (dataset->widget() == handle)
                widgets.append(dataset);
        }
    }

    return widgets;
}

/**
 * Returns the titles of the datasets contained in the specified @a vector.
 */
QStringList Dashboard::datasetTitles(const QVector<JSON::Dataset *> &vector) const
{
    QStringList list;
    foreach (auto set, vector)
        list.append(set->title());

    return list;
}

/**
 * Returns the titles of the groups contained in the specified @a vector.
 */
QStringList Dashboard::groupTitles(const QVector<JSON::Group *> &vector) const
{
    QStringList list;
    foreach (auto group, vector)
        list.append(group->title());

    return list;
}

/**
 * Returns @c true if the widget at the specifed @a index of the @a vector should be
 * displayed in the QML user interface.
 */
bool Dashboard::getVisibility(const QVector<bool> &vector, const int index) const
{
    if (index < vector.count())
        return vector[index];

    return false;
}

/**
 * Changes the @a visible flag of the widget at the specified @a index of the given @a
 * vector. Calling this function with @a visible set to @c false will hide the widget in
 * the QML user interface.
 */
void Dashboard::setVisibility(QVector<bool> &vector, const int index, const bool visible)
{
    if (index < vector.count())
    {
        vector[index] = visible;
        emit widgetVisibilityChanged();
    }
}