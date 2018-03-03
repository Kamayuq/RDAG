#include <set>
#include "GraphCulling.h"
#include "Plumber.h"
#include "Renderpass.h"

void GraphProcessor::ColorGraphNodesInternal(const IRenderPassAction* Action, std::vector<const IRenderPassAction*>& InAllActions, U32 ParentColor)
{
	const auto* Pass = Action->GetRenderPassData();

	U32 NumValidMutables = 0;
	for (const ResourceTableEntry& Output : Pass->AsOutputIterator())
	{
		if (Output.IsMaterialized())
		{
			NumValidMutables++;
			if (const IRenderPassAction* Parent = Output.GetParent() ? Output.GetParent()->GetAction() : nullptr)
			{
				auto Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
				if (Iter != InAllActions.end() && *Iter != Action)
				{
					ColorGraphNodesInternal(*Iter, InAllActions, GetNewColor(ParentColor));
					Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
					if (Iter != InAllActions.end())
					{
						InAllActions.erase(Iter);
					}
				}
			}
		}
	}

	if (NumValidMutables)
	{
		//Statics only need to be processed for Leafs
		const IRenderPassAction* Action = Pass->GetAction();
		if (Action != nullptr)
		{
			for (const ResourceTableEntry& Input : Pass->AsInputIterator())
			{
				if (Input.IsValid())
				{
					if (const IRenderPassAction* Parent = Input.GetParent() ? Input.GetParent()->GetAction() : nullptr)
					{
						Input.Materialize();
						auto Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
						if (Iter != InAllActions.end() && *Iter != Action)
						{
							ColorGraphNodesInternal(*Iter, InAllActions, GetNewColor(ParentColor));
							Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
							if (Iter != InAllActions.end())
							{
								InAllActions.erase(Iter);
							}
						}
					}
				}
			}

			if (Action->GetColor() == UINT32_MAX || Action->GetColor() < ParentColor)
			{
				Action->SetColor(ParentColor);
			}
		}
	}
}