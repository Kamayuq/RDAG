#pragma once
#include <vector>
#include <algorithm>

#include "Renderpass.h"
#include "Types.h"

struct LeafRenderPass;

struct GraphProcessor
{
	void ColorGraphNodes(const std::vector<const IRenderPassAction*>& InAllActions)
	{
		if (InAllActions.size() > 1)
		{
			std::vector<const IRenderPassAction*> AllActions;
			AllActions.assign(InAllActions.begin(), InAllActions.end());
			std::sort(AllActions.begin(), AllActions.end(), std::less<const IRenderPassAction*>());
			ColorGraphNodesInternal(InAllActions.back(), AllActions, 0);
		}
	}

	//void ScheduleGraphNodes(const struct IRenderPassAction* Pass, std::vector<const IRenderPassAction*>& OutPasses) const;

private:
	void ColorGraphNodesInternal(const IRenderPassAction* Action, std::vector<const IRenderPassAction*>& InAllActions, U32 ParentColor);

	U32 GetNewColor(U32 OldColor)
	{
		return OldColor + 1 < UINT_MAX ? OldColor + 1 : 0;
	}
};