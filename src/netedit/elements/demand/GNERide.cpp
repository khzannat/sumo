/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.dev/sumo
// Copyright (C) 2001-2023 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNERide.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Jun 2019
///
// A class for visualizing rides in Netedit
/****************************************************************************/

#include <utils/gui/windows/GUIAppEnum.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNENet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNEViewNet.h>
#include <utils/gui/div/GUIDesigns.h>

#include "GNERide.h"

// ===========================================================================
// method definitions
// ===========================================================================

GNERide*
GNERide::buildRide(GNENet* net, GNEDemandElement* personParent,
        GNEEdge* fromEdge, GNEAdditional* fromBusStop, GNEAdditional* fromTrainStop,
        GNEEdge* toEdge, GNEAdditional* toBusStop, GNEAdditional* toTrainStop,
        double arrivalPosition, const std::vector<std::string>& lines) {
    // declare icon an tag
    const auto iconTag = getRideTagIcon(fromEdge, toEdge, fromBusStop, toBusStop, fromTrainStop, toTrainStop);
    // declare containers
    std::vector<GNEJunction*> junctions;
    std::vector<GNEEdge*> edges;
    std::vector<GNEAdditional*> additionals;
    // continue depending of input parameters
    if (fromEdge) {
        edges.push_back(fromEdge);
    } else if (fromBusStop) {
        additionals.push_back(fromBusStop);
    } else if (fromTrainStop) {
        additionals.push_back(fromTrainStop);
    }
    if (toEdge) {
        edges.push_back(toEdge);
    } else if (toBusStop) {
        additionals.push_back(toBusStop);
    } else if (toTrainStop) {
        additionals.push_back(toTrainStop);
    }
    return new GNERide(net, iconTag.first, iconTag.second, personParent, edges, additionals,arrivalPosition, lines);
}


GNERide::GNERide(SumoXMLTag tag, GNENet* net) :
    GNEDemandElement("", net, GLO_RIDE, tag, GUIIconSubSys::getIcon(GUIIcon::RIDE_EDGE),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {}, {}, {}, {}, {}),
    GNEDemandElementPlan(this, -1, -1) {
    // reset default values
    resetDefaultValues();
}


GNERide::~GNERide() {}


GNEMoveOperation*
GNERide::getMoveOperation() {
    return getPlanMoveOperation();
}


GUIGLObjectPopupMenu*
GNERide::getPopUpMenu(GUIMainWindow& app, GUISUMOAbstractView& parent) {
    return getPlanPopUpMenu(app, parent);
}


void
GNERide::writeDemandElement(OutputDevice& device) const {
    // open tag
    device.openTag(SUMO_TAG_RIDE);
    // write plan attributes
    writePlanAttributes(device);
    // write lines
    if (myLines.empty()) {
        device.writeAttr(SUMO_ATTR_LINES, "ANY");
    } else {
        device.writeAttr(SUMO_ATTR_LINES, myLines);
    }
    // close tag
    device.closeTag();
}


GNEDemandElement::Problem
GNERide::isDemandElementValid() const {
    return isPersonPlanValid();
}


std::string
GNERide::getDemandElementProblem() const {
    return getPersonPlanProblem();
}


void
GNERide::fixDemandElementProblem() {
    // currently the only solution is removing Ride
}


SUMOVehicleClass
GNERide::getVClass() const {
    return getParentDemandElements().front()->getVClass();
}


const RGBColor&
GNERide::getColor() const {
    return myNet->getViewNet()->getVisualisationSettings().colorSettings.rideColor;
}


void
GNERide::updateGeometry() {
    updatePlanGeometry();
}


Position
GNERide::getPositionInView() const {
    return getPlanPositionInView();
}


std::string
GNERide::getParentName() const {
    return getParentDemandElements().front()->getID();
}


Boundary
GNERide::getCenteringBoundary() const {
    return getPlanCenteringBoundary();
}


void
GNERide::splitEdgeGeometry(const double /*splitPosition*/, const GNENetworkElement* /*originalElement*/, const GNENetworkElement* /*newElement*/, GNEUndoList* /*undoList*/) {
    // geometry of this element cannot be splitted
}


void
GNERide::drawGL(const GUIVisualizationSettings& s) const {
    drawPlanGL(checkDrawPersonPlan(), s, s.colorSettings.rideColor, s.colorSettings.selectedPersonPlanColor);
}


void
GNERide::computePathElement() {
    // get first and last path lanes
    const auto firstPathLane = getFirstPathLane();
    const auto lastPathLane = getLastPathLane();
    // continue depending of pathLane
    if (firstPathLane && lastPathLane) {
        const std::vector<GNELane*> lanes = {firstPathLane, lastPathLane};
        // calculate path
        myNet->getPathManager()->calculatePathLanes(this, SVC_PASSENGER, lanes);
        // check path (taxis)
        if (!myNet->getPathManager()->isPathValid(this)) {
            myNet->getPathManager()->calculatePathLanes(this, SVC_TAXI, lanes);
        }
        // check path (bus)
        if (!myNet->getPathManager()->isPathValid(this)) {
            myNet->getPathManager()->calculatePathLanes(this, SVC_BUS, lanes);
        }
        // check path (bicycle)
        if (!myNet->getPathManager()->isPathValid(this)) {
            myNet->getPathManager()->calculatePathLanes(this, SVC_BICYCLE, lanes);
        }
        // check path (pedestrian)
        if (!myNet->getPathManager()->isPathValid(this)) {
            myNet->getPathManager()->calculatePathLanes(this, SVC_PEDESTRIAN, lanes);
        }
    } else {
        // reset path
        myNet->getPathManager()->calculatePathLanes(this, SVC_PEDESTRIAN, {});
    }
    // update geometry
    updateGeometry();
}


void
GNERide::drawPartialGL(const GUIVisualizationSettings& s, const GNELane* lane, const GNEPathManager::Segment* segment, const double offsetFront) const {
    drawPlanPartial(checkDrawPersonPlan(), s, lane, segment, offsetFront, s.widthSettings.rideWidth, s.colorSettings.rideColor, s.colorSettings.selectedPersonPlanColor);
}


void
GNERide::drawPartialGL(const GUIVisualizationSettings& s, const GNELane* fromLane, const GNELane* toLane, const GNEPathManager::Segment* segment, const double offsetFront) const {
    drawPlanPartial(checkDrawPersonPlan(), s, fromLane, toLane, segment, offsetFront, s.widthSettings.rideWidth, s.colorSettings.rideColor, s.colorSettings.selectedPersonPlanColor);
}


GNELane*
GNERide::getFirstPathLane() const {
    // get previous plan
    const auto previousPlan = getParentDemandElements().at(0)->getPreviousChildDemandElement(this);
    if (previousPlan) {
        // sue the last pathLane of previous plan
        return previousPlan->getLastPathLane();
    } else {
        return getParentEdges().front()->getLaneByDisallowedVClass(SVC_PEDESTRIAN);
    }
}


GNELane*
GNERide::getLastPathLane() const {
    // check if personPlan ends in a BusStop
    if (getParentAdditionals().size() > 0) {
        return getParentAdditionals().front()->getParentLanes().front();
    } else {
        return getParentEdges().back()->getLaneByDisallowedVClass(SVC_PEDESTRIAN);
    }
}


std::string
GNERide::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_LINES:
            return joinToString(myLines, " ");
        default:
            return getPlanAttribute(key);
    }
}


double
GNERide::getAttributeDouble(SumoXMLAttr key) const {
    return getPlanAttributeDouble(key);
}


Position
GNERide::getAttributePosition(SumoXMLAttr key) const {
    return getPlanAttributePosition(key);
}


void
GNERide::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    switch (key) {
        case SUMO_ATTR_LINES:
            GNEChange_Attribute::changeAttribute(this, key, value, undoList);
            break;
        default:
            setPlanAttribute(key, value, undoList);
            break;
    }
}


bool
GNERide::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_LINES:
            return canParse<std::vector<std::string> >(value);
        default:
            return isPlanValid(key, value);
    }
}


bool
GNERide::isAttributeEnabled(SumoXMLAttr key) const {
    return isPlanAttributeEnabled(key);
}


std::string
GNERide::getPopUpID() const {
    return getTagStr();
}


std::string
GNERide::getHierarchyName() const {
    return getPlanHierarchyName();
}


const Parameterised::Map&
GNERide::getACParametersMap() const {
    return getParametersMap();
}

// ===========================================================================
// private
// ===========================================================================

void
GNERide::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_LINES:
            myLines = GNEAttributeCarrier::parse<std::vector<std::string> >(value);
            break;
        default:
            setPlanAttribute(key, value);
            break;
    }
}


void
GNERide::setMoveShape(const GNEMoveResult& moveResult) {
    // change both position
    myArrivalPosition = moveResult.newSecondPos;
    // update geometry
    updateGeometry();
}


void
GNERide::commitMoveShape(const GNEMoveResult& moveResult, GNEUndoList* undoList) {
    undoList->begin(this, "arrivalPos of " + getTagStr());
    // now adjust start position
    setAttribute(SUMO_ATTR_ARRIVALPOS, toString(moveResult.newFirstPos), undoList);
    undoList->end();
}


GNERide::GNERide(GNENet* net, SumoXMLTag tag, GUIIcon icon, GNEDemandElement* personParent, const std::vector<GNEEdge*> &edges,
                 const std::vector<GNEAdditional*> &additionals, double arrivalPosition, const std::vector<std::string>& lines) :
    GNEDemandElement(personParent, net, GLO_PERSONTRIP, tag, GUIIconSubSys::getIcon(icon),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, edges, {}, additionals, {personParent}, {}),
    GNEDemandElementPlan(this, -1, arrivalPosition),
    myLines(lines) {
}

/****************************************************************************/