//
// Created by Elevoc on 2021/8/20 0020.
//

#ifndef ELEVOC_ENGINE_EV_EXPORT_H
#define ELEVOC_ENGINE_EV_EXPORT_H

#if defined(_MSC_VER)
//  Microsoft
#define EVEXPORT __declspec(dllexport)
#define EVIMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define EVEXPORT __attribute__((visibility("default")))
#define EVIMPORT
#else
//  do nothing and hope for the best?
#define DNNEXPORT
#define DNNIMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif
#endif //ELEVOC_ENGINE_EV_EXPORT_H
