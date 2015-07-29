#include "inputHandler.h"

#include <cmath>
#include "glm/glm.hpp"
#include "platform.h"

namespace Tangram {

InputHandler::InputHandler(std::shared_ptr<View> _view) : m_view(_view) {
    
    m_deltaZoom = 0.f;
    m_deltaTranslate = glm::vec2(0.f);
    
}

void InputHandler::update(float _dt) {
    
    if (!m_gestureOccured) {
        
        if (glm::length(m_deltaTranslate) > m_minDeltaLength || std::abs(m_deltaZoom) > m_minDeltaZoomLength) {

            static std::bitset<8> pan(1 << GestureFlags::pan);
            static std::bitset<8> pinch(1 << GestureFlags::pinch);

            // exponential decrease, m_expDecrease should be between [0..1]
            if (pan == m_gestures || m_momentumHandled) {
                m_deltaTranslate *= m_expDecrease;
                m_view->translate(m_deltaTranslate.x, m_deltaTranslate.y);
                m_momentumHandled = true;
            }
            
            // to give more control, make zoom decrease faster than translation
            if (pinch == m_gestures || m_momentumHandled) {
                m_deltaZoom *= m_expDecrease * m_expDecrease;
                m_view->zoom(m_deltaZoom);
                m_momentumHandled = true;
            }

            requestRender();
        } else {

            m_momentumHandled = false;
        }
        
        m_gestures.reset();
    }

    m_gestureOccured = false;
}

void InputHandler::handleTapGesture(float _posX, float _posY) {

    clearDeltas();
    m_gestures.set(GestureFlags::tap);
    
    float viewCenterX = 0.5f * m_view->getWidth();
    float viewCenterY = 0.5f * m_view->getHeight();

    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);

    m_view->translate((_posX - viewCenterX), (_posY - viewCenterY));

    onEndGesture();
}

void InputHandler::handleDoubleTapGesture(float _posX, float _posY) {

    clearDeltas();
    m_gestures.set(GestureFlags::double_tap);
    
    handlePinchGesture(_posX, _posY, 2.f, 0.f);
}

void InputHandler::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    // The platform can send the same values for the last registered pan
    if (std::abs(_startX - _endX) < FLT_EPSILON && std::abs(_startY - _endY) < FLT_EPSILON) {
        return;
    }
    
    clearDeltas();
    m_gestures.set(GestureFlags::pan);
    
    float dScreenX = _startX - _endX;
    float dScreenY = _startY - _endY;

    m_view->screenToGroundPlane(_startX, _startY);
    m_view->screenToGroundPlane(_endX, _endY);

    float dx = _startX - _endX;
    float dy = _startY - _endY;

    if (glm::length(glm::vec2(dScreenX, dScreenY)) > m_minDeltaTranslate) {
        setDeltas(0.f, glm::vec2(dx, dy));
    }

    m_view->translate(dx, dy);

    onEndGesture();
}

void InputHandler::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    clearDeltas();
    m_gestures.set(GestureFlags::pinch);

    m_view->screenToGroundPlane(_posX, _posY);

    m_view->undoFocusPosition(glm::vec2(_posX, _posY), mPrevFocus);
    mPrevFocus = glm::vec2(_posX, _posY);

    static float invLog2 = 1 / log(2);

    setDeltas(m_minZoomStart * _velocity, glm::vec2(0.f));

    m_view->zoom(log(_scale) * invLog2);

    onEndGesture();
}

void InputHandler::handleRotateGesture(float _posX, float _posY, float _radians) {
    
    clearDeltas();
    m_gestures.set(GestureFlags::rotate);
    
    m_view->screenToGroundPlane(_posX, _posY);

    m_view->undoFocusPosition(glm::vec2(_posX, _posY), mPrevFocus);
    mPrevFocus = glm::vec2(_posX, _posY);

    m_view->roll(_radians);

    onEndGesture();
}

void InputHandler::handleShoveGesture(float _distance) {
    
    clearDeltas();
    m_gestures.set(GestureFlags::shove);
    
    m_view->pitch(_distance);

    onEndGesture();
}

void InputHandler::handlePinchGestureEnd() {
    mPrevFocus = glm::vec2(0.0f, 0.0f);
}

void InputHandler::handleRotateGestureEnd() {
    mPrevFocus = glm::vec2(0.0f, 0.0f);
}

void InputHandler::onEndGesture() {

    m_gestureOccured = true;

    // request a frame
    requestRender();
}
    
void InputHandler::clearDeltas() {
    m_deltaZoom = 0.f;
    m_deltaTranslate = glm::vec2(0.f);
}

void InputHandler::setDeltas(float _zoom, glm::vec2 _translate) {

    // setup deltas for momentum on gesture
    m_deltaTranslate = _translate;
    m_deltaZoom = _zoom;
}

}
