#include "GraphCulling.h"
#include "Plumber.h"
#include "Renderpass.h"

bool GraphProcessor::ColorGraphNodesInternal(const IRenderPassAction* Action, std::vector<const IRenderPassAction*>& InAllActions)
{
	const IResourceTableInfo& Pass = Action->GetRenderPassData();
	bool isFirstPath = true;

	U32 NumValidMutables = 0;
	for (const ResourceTableEntry& Output : Pass)
	{
		if (Output.IsOutput() && Output.IsMaterialized())
		{
			NumValidMutables++;
		}
	}

	//Statics only need to be processed for Leafs
	if (Action != nullptr)
	{
		for (const ResourceTableEntry& Input : Pass)
		{
			if (!Input.IsUndefined())
			{
				if (NumValidMutables)
				{
					Input.Materialize();
				}

				if (const IRenderPassAction* Parent = Input.GetParent() ? Input.GetParent()->GetAction() : nullptr)
				{
					auto Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
					if (Iter != InAllActions.end() && *Iter != Action)
					{
						if (!isFirstPath)
						{
							NextColor();
						}

						isFirstPath &= ColorGraphNodesInternal(*Iter, InAllActions);
						Iter = std::find(InAllActions.begin(), InAllActions.end(), Parent);
						if (Iter != InAllActions.end())
						{
							InAllActions.erase(Iter);
						}
					}
				}
			}
		}

		if (NumValidMutables && (Action->GetColor() == UINT32_MAX || Action->GetColor() < CurrentColor))
		{
			Action->SetColor(CurrentColor);
			return false;
		}
	}

	return isFirstPath;
}