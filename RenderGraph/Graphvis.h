#pragma once
#include <vector>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include "Plumber.h"
#include "Renderpass.h"

struct ColorStyle
{
	enum Values                                              {  Grey,     Black,  Mangenta,   Red,   Orange,   Gold,   Darkgreen,   Blue,   Darkturquoise,   Brown,   Coral,   Purple,   Yellowgreen,   Indigo,   MaxValues };
	static constexpr const char* ColorChart[MaxValues + 1] = { "grey95", "black", "magenta", "red", "orange", "gold", "darkgreen", "blue", "darkturquoise", "brown", "coral", "purple", "yellowgreen", "indigo", "MaxValues" };

public:
	ColorStyle(U32 Index) : Value(Values(1 + (Index % (MaxValues - 1)))) {}
	ColorStyle(Values InValue) : Value(InValue) {}

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(color="%s")", ColorChart[Value]);
	}

private:
	Values Value;
};

struct FontColorStyle : private ColorStyle
{
	using ColorStyle::ColorStyle;

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(font)");
		ColorStyle::Print(fhp);
	}
};

struct DrawStyle
{
	enum Styles                                 {  Invis,   Dotted,   Solid,   Tapered };
	static constexpr const char* StyleChart[] = { "invis", "dotted", "solid", "tapered" };

	enum Shapes                                 {  Hexagon,   Ellipse,   Rectangle };
	static constexpr const char* ShapeChart[] = { "hexagon", "ellipse", "rectangle"};

	DrawStyle(Shapes InShapeValue, Styles InStyleValue, U32 InPenWidth = 1u) : ShapeValue(InShapeValue), StyleValue(InStyleValue), PenWidth(InPenWidth) {}

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(shape="%s", style="%s", penwidth=%d)", ShapeChart[ShapeValue], StyleChart[StyleValue], PenWidth);
	}

private:
	Shapes ShapeValue;
	Styles StyleValue;
	U32 PenWidth;
};

struct ArrowStyle
{
	enum Heads					                { Normal,   Inv,   None,   Open };
	static constexpr const char* HeadChart[] = { "normal", "inv", "none", "open" };

	enum Dirs								   { Forward,   Back,   Both  };
	static constexpr const char* DirChart[] = { "forward", "back", "both" };

	ArrowStyle(Dirs InDir, Heads InHead, Heads InTail) : Direction(InDir), Head(InHead), Tail(InTail) {}

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(dir="%s", arrowhead="%s", arrowtail="%s")", DirChart[Direction], HeadChart[Head], HeadChart[Tail]);
	}

private:
	Dirs Direction;
	Heads Head;
	Heads Tail;
};

inline U32 CalculateRank(const IResourceTableInfo* Table)
{
	U32 Rank = 0;
	const IResourceTableInfo* Parent = Table->GetParent();
	while (Parent)
	{
		Rank++;
		Parent = Parent->GetParent();
	}
	return Rank;
}

inline bool HasSameRankAndName(const IResourceTableInfo* Owner, const IResourceTableInfo* Parent)
{
	return CalculateRank(Owner) == CalculateRank(Parent) && (strcmp(Owner->GetName(), Parent->GetName()) == 0);
}

struct PinStyle
{
	enum Type { Input, Output };
	static constexpr const char* TypeChart[2] = { "Input", "Output" };

	PinStyle(Type InType, const ResourceTableEntry& InEntry, const IRenderPassAction* InRenderPassAction) 
		: EntryType(InType), Entry(InEntry), RenderPassAction(InRenderPassAction)
		, ArrowDrawStyle(ArrowStyle::Forward, ArrowStyle::Normal, ArrowStyle::None)
		, PinDrawStyle(DrawStyle::Ellipse, DrawStyle::Solid)
		, PinColorStyle(ColorStyle::Grey)
		, PinFontColorStyle(ColorStyle::Grey)
	{
		if (!Entry.IsValid())
		{
			PinDrawStyle = DrawStyle(DrawStyle::Ellipse, DrawStyle::Invis);
		}
		else if (Entry.IsExternal())
		{
			PinDrawStyle = DrawStyle(DrawStyle::Hexagon, DrawStyle::Solid);
		}
		else if (Entry.GetParent() == nullptr)
		{
			PinDrawStyle = DrawStyle(DrawStyle::Ellipse, DrawStyle::Dotted);
		}

		bool IsMaterialized = (InRenderPassAction->GetColor() != UINT32_MAX) && InEntry.IsMaterialized();
		if (IsMaterialized )
		{
			PinColorStyle = ColorStyle(RenderPassAction->GetColor());
			PinFontColorStyle = FontColorStyle(RenderPassAction->GetColor());
		}
	}

	void PrintName(FILE* fhp) const
	{
		fprintf(fhp, R"(%s%zu)", TypeChart[EntryType], Entry.Hash());
	}

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(
			)");
		PrintName(fhp);
		fprintf(fhp, R"([)");
		PinDrawStyle.Print(fhp); fprintf(fhp, ", ");
		PinColorStyle.Print(fhp); fprintf(fhp, ", ");
		PinFontColorStyle.Print(fhp);
		fprintf(fhp, R"(, label="%s\n%s")", TypeChart[EntryType], Entry.GetName());
		fprintf(fhp, R"(];)");
	}

	void DebugPrint(FILE* fhp) const
	{
		fprintf(fhp, 
			R"(//EntryInfoName: %s Immaginary: 0x%I64X Owner: %s Parent: %s)", 
			Entry.GetName(), (UintPtr)Entry.GetImaginaryResource(), Entry.GetOwner()->GetName(), Entry.GetParent() ? Entry.GetParent()->GetName() : "Orphan");
	}

	void DrawArrow(FILE* fhp) const
	{
		check(EntryType == Input);
		if (Entry.GetParent() == nullptr)
			return;

		//bool SameRank = HasSameRankAndName(Entry.GetParent(), Entry.GetOwner());
		fprintf(fhp, R"(
		Output%zu -> Input%zu [constraint = true, penwidth = 2, )", Entry.ParentHash(), Entry.Hash());
		PinColorStyle.Print(fhp);
		fprintf(fhp, R"(];)");
	}
	
	const void* GetImaginaryResource() const
	{
		return Entry.GetImaginaryResource();
	}

private:
	Type EntryType;
	ResourceTableEntry Entry;
	const IRenderPassAction* RenderPassAction;

private:
	ArrowStyle ArrowDrawStyle;
	DrawStyle PinDrawStyle;
	ColorStyle PinColorStyle;
	FontColorStyle PinFontColorStyle;
};

struct ActionStyle
{
	ActionStyle(const IRenderPassAction* InRenderPassAction) : RenderPassAction(InRenderPassAction), LocalActionIndex(GlobalActionIndex++)
	{
		Rank = CalculateRank(RenderPassAction->GetRenderPassData());

		for (const auto& Entry : RenderPassAction->GetRenderPassData()->AsInputIterator())
		{
			InputPins.push_back(PinStyle(PinStyle::Input, Entry, RenderPassAction));
		}
		std::sort(InputPins.begin(), InputPins.end(), [](const PinStyle& a, const PinStyle& b)
		{
			return a.GetImaginaryResource() < b.GetImaginaryResource();
		});

		for (const auto& Entry : RenderPassAction->GetRenderPassData()->AsOutputIterator())
		{
			OutputPins.push_back(PinStyle(PinStyle::Output, Entry, RenderPassAction));
		}
		std::sort(OutputPins.begin(), OutputPins.end(), [](const PinStyle& a, const PinStyle& b)
		{
			return a.GetImaginaryResource() < b.GetImaginaryResource();
		});
	}

	void PrintPins(FILE* fhp, const std::vector<PinStyle>& Pins, const char* Rank) const
	{
		for (const auto& Entry : Pins)
		{
			fprintf(fhp, R"(
			)");
			Entry.DebugPrint(fhp);
			Entry.Print(fhp);
		}

		fprintf(fhp, R"(
			{rank = %s; )", Rank);
		for (const auto& Entry : Pins)
		{
			Entry.PrintName(fhp);
			fprintf(fhp, R"( ; )");
		}
		fprintf(fhp, R"(};)");
	}

	void PrintName(FILE* fhp) const
	{
		fprintf(fhp, R"(Clust%i)", LocalActionIndex);
	}

	void Print(FILE* fhp) const
	{
		fprintf(fhp, R"(
		subgraph "cluster_%i"	 
		{	
			label="%s";
			color="grey";
			fillcolor="litegrey"
			)", LocalActionIndex, RenderPassAction->GetName());
		PrintName(fhp);
		fprintf(fhp, R"([style = "invis"])");
		
		PrintPins(fhp, InputPins, "same");
		PrintPins(fhp, OutputPins, "same");

		if (InputPins.size() > 0 && OutputPins.size() > 0)
		{
			fprintf(fhp, R"(
			)");
			InputPins[0].PrintName(fhp);
			fprintf(fhp, " -> ");
			OutputPins[0].PrintName(fhp);
			fprintf(fhp, R"([style = "invis"])");
		}

		fprintf(fhp, R"(
		})");
	}

	U32 GetRank() const
	{
		return Rank;
	}

	const IRenderPassAction* GetRenderPassAction() const
	{
		return RenderPassAction;
	}

private:
	const IRenderPassAction* RenderPassAction;
	
	std::vector<PinStyle> InputPins;
	std::vector<PinStyle> OutputPins;

private:
	U32 Rank = 0;
	U32 LocalActionIndex = 0;

	static U32 GlobalActionIndex;
};


struct GraphvisNeoWriter
{
public:
	GraphvisNeoWriter(const char* FileName, const std::vector<const IRenderPassAction*>& InAllActions)
	{
		fhp = fopen(FileName, "w+");

		fprintf(fhp, R"(
digraph G
{
	graph[remincross = true, compound = true, concentrate = false, rankdir = TB];
	//newrank = true;
	pack = true;			
	packMode = "clust";	
	clusterrank="local"	
	outputorder="breadthfirst")");

		for (const IRenderPassAction* Action : InAllActions)
		{
			Actions.push_back(ActionStyle(Action));
			for (ResourceTableEntry Entry : Action->GetRenderPassData()->AsInputIterator())
			{
				AllInputEntries.push_back(PinStyle(PinStyle::Input, Entry, Action));
			}
		}
		std::sort(Actions.begin(), Actions.end(), [](const ActionStyle& a, const ActionStyle& b)
		{
			return a.GetRank() < b.GetRank();
		});

		PrintActions(fhp, "same");

		for (const auto& PinEntry : AllInputEntries)
		{
			PinEntry.DrawArrow(fhp);
		}
	}

	void PrintActions(FILE* fhp, const char* Rank) const
	{
		if (Actions.size() == 0)
			return;

		for (const auto& Action : Actions)
		{
			Action.Print(fhp);
		}

		const IResourceTableInfo* Previous = Actions[0].GetRenderPassAction()->GetRenderPassData();
		for (U32 i = 0; i < Actions.size(); i++)
		{
			auto Action = Actions[i];
			fprintf(fhp, R"(
		{rank = %s; )", Rank);

			bool SameRank = true;
			while (SameRank && i < Actions.size())
			{
				Action = Actions[i];
				Action.PrintName(fhp);
				fprintf(fhp, R"( ; )");
				i++;
				SameRank = HasSameRankAndName(Previous, Action.GetRenderPassAction()->GetRenderPassData());
				Previous = Action.GetRenderPassAction()->GetRenderPassData();
			}

			fprintf(fhp, R"(};)");
		}
	}

	~GraphvisNeoWriter()
	{

		fprintf(fhp, R"(
})");
		fclose(fhp);
	}

private:
	std::vector<PinStyle> AllInputEntries;
	std::vector<ActionStyle> Actions;
	FILE* fhp = nullptr;
};