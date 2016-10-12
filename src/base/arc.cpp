//
// Created by alberto on 12/10/16.
//

#include "arc.h"

namespace mvrp {
    std::ostream& operator<<(std::ostream& out, const ArcType& arctype) {
        if(arctype == ArcType::PORT_TO_PORT) {
            out << "port-to-port";
        } else if(arctype == ArcType::PORT_TO_COMEBACK_HUB) {
            out << "port-to-comeback-hub";
        } else if(arctype == ArcType::SOURCE_TO_PORT) {
            out << "source-to-port";
        } else if(arctype == ArcType::PORT_TO_SINK) {
            out << "port-to-sink";
        } else if(arctype == ArcType::COMEBACK_HUB_TO_PORT) {
            out << "comeback-hub-to-port";
        } else if(arctype == ArcType::DELIVERY_TO_PICKUP) {
            out << "delivery-to-pickup";
        } else {
            out << "unknown";
        }

        return out;
    }
}