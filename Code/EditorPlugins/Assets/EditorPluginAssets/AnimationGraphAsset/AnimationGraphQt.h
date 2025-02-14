#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class plQtAnimationGraphNode : public plQtNode
{
public:
  plQtAnimationGraphNode();

  virtual void UpdateState() override;
};
