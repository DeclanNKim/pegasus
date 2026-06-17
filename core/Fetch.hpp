#pragma once

#include "core/ActionGroup.hpp"
#include "core/translate/ExecutionPage.hpp"

#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/Unit.hpp"

#include <unordered_map>
#include <memory>
#include <tuple>

namespace pegasus
{
    class PegasusState;

    class Fetch : public sparta::Unit
    {
      public:
        // Name of this resource, required by sparta::UnitFactory
        static constexpr char name[] = "Fetch";
        using base_type = Fetch;

        class FetchParameters : public sparta::ParameterSet
        {
          public:
            FetchParameters(sparta::TreeNode* node) : sparta::ParameterSet(node) {}

            // Runtime parameter that allows for the execution cache to be used
            PARAMETER(bool, enable_execution_cache, false,
                      "Enable the experimental translated-page execution cache fast path")
        };

        Fetch(sparta::TreeNode* fetch_node, const FetchParameters* p);

        ActionGroup* getActionGroup() { return &fetch_action_group_; }

        // Conservative full flush of translated execution pages.
        void flushExecutionCache();

      private:
        PegasusState* state_ = nullptr;
        const bool enable_execution_cache_ = false;
        ActionGroup* execute_action_group_ = nullptr;

        // ExecutionPageKey is a tuple of (virt_page_base_addr, phys_page_base_addr, page_size)
        // It allows us to identify an execution page
        using ExecutionPageKey = std::tuple<Addr, Addr, Addr>;

        struct ExecutionPageKeyHash
        {
            size_t operator()(const ExecutionPageKey & k) const noexcept
            {
                // FNV-style hash combine over the three Addr fields
                size_t h = std::get<0>(k);
                h ^= std::get<1>(k) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                h ^= std::get<2>(k) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                return h;
            }
        };

        // The actual map of execution pages, keyed by the above tuple
        std::unordered_map<ExecutionPageKey, std::unique_ptr<ExecutionPage>, ExecutionPageKeyHash>
            execution_pages_;

        void onBindTreeEarly_() override;

        Action::ItrType fetch_(pegasus::PegasusState* state, Action::ItrType action_it);

        ActionGroup fetch_action_group_{"Fetch"};

        // Action group for dispatching a fetch that has already been translated to an ExecutionPage
        Action::ItrType dispatchTranslatedFetch_(pegasus::PegasusState* state,
                                                Action::ItrType action_it);

        ActionGroup translated_fetch_dispatch_action_group_{"Dispatch Translated Fetch"};

        Action::ItrType decode_(pegasus::PegasusState* state, Action::ItrType action_it);

        ActionGroup decode_action_group_{"Decode"};
    };
} // namespace pegasus
