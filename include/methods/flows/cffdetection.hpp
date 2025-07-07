#ifndef METHODS_FLOWS_CFF_DETECTION_HPP
#define METHODS_FLOWS_CFF_DETECTION_HPP

#pragma once

#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cmath>

#include "binaryninjaapi.h"
#include "../../deobfuscator.hpp"

using namespace BinaryNinja;

namespace Flows
{
    class CFFDetection : public IDeobfuscationMethod
    {
    public:
        CFFDetection();

        // === Cyclomatic Flow Flattening Analysis ===
        
        // Retourne les blocs dominés par un BasicBlock donné
        std::set<Ref<BasicBlock>> getDominatedBy(const Ref<BasicBlock>& dominator);

        // Calcule le score de flattening basé sur la structure de domination
        double CalcFlatteningScore(const Ref<Function>& function);

        // Calcule la complexité cyclomatique (McCabe)
        int CalcCyclomaticComplexity(Ref<Function> func);

        // === Dispatcher/State Variable Analysis ===

        // Retourne la variable la plus assignée dans une fonction HLIL
        Variable GetMostAssignedVar(const Ref<Function>& func);

        // Analyse les dépendances d'une variable (ex. transitions de dispatcher)
        std::vector<Variable> GetVarDependencies(const Ref<HighLevelILFunction>& hlilFunc, const Variable& var);

        // === Méthode principale appelée par le framework ===
        void execute(const Ref<AnalysisContext>& analysisContext) override;
    };
}

#endif // METHODS_FLOWS_CFF_DETECTION_HPP