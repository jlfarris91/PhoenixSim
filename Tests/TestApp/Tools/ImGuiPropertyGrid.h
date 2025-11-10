

#pragma once

#include "Reflection.h"

void DrawPropertyEditor(void* obj, const Phoenix::PropertyDescriptor& propertyDesc);
void DrawPropertyEditor(const void* obj, const Phoenix::PropertyDescriptor& propertyDesc);

void DrawPropertyGrid(void* obj, const Phoenix::TypeDescriptor& descriptor);
void DrawPropertyGrid(const void* obj, const Phoenix::TypeDescriptor& descriptor);
