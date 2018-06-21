#include "GraphCulling.h"
#include "Plumber.h"
#include "Renderpass.h"
#include <unordered_map>

void GraphProcessor::ColorGraphNodesInternal(const IRenderPassAction* Action, std::vector<const IRenderPassAction*>& InAllActions, U32 ParentColor)
{
	const IResourceTableInfo& Pass = Action->GetRenderPassData();

	U32 NumValidMutables = 0;
	if (ParentColor != UINT32_MAX)
	{
		for (const ResourceTableEntry& Output : Pass)
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
	}

	if (Action != nullptr)
	{
		for (const ResourceTableEntry& Input : Pass)
		{
			if (!Input.IsUndefined())
			{
				if (const IRenderPassAction* Parent = Input.GetParent() ? Input.GetParent()->GetAction() : nullptr)
				{
					if (NumValidMutables)
					{
						//Statics only need to be processed for Leafs
						Input.Materialize();
					}

					auto Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
					if (Iter != InAllActions.end() && *Iter != Action)
					{
						ColorGraphNodesInternal(*Iter, InAllActions, NumValidMutables ? GetNewColor(ParentColor) : UINT32_MAX);
						Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
						if (Iter != InAllActions.end())
						{
							InAllActions.erase(Iter);
						}
					}
				}
			}
		}

		if ((Action->GetColor() == UINT32_MAX || Action->GetColor() < ParentColor) && ParentColor != UINT32_MAX)
		{
			Action->SetColor(ParentColor);
		}
	}
}