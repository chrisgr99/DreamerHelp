/** A STARTING HELP FILE FOR EVERY MODULE OF A PLUGIN — see "Exporting a starting file" in
design/help-database.md.

So that a maker or a user does not start from an empty file. Each file lists every parameter,
input, output and light the module reports, with the name the module gives it. Each entry's own
text is left empty, and the database's text for it is put beside it under "database", which the
reader ignores. So an entry somebody leaves alone falls back to the database instead of freezing
today's text in front of tomorrow's, and an entry they fill in is theirs.

Controls the database describes with one sentence stay one entry naming all of them, as the
database has them. A file already in the folder is never written over: exporting twice must not
throw away what somebody has written since the first time.
*/
#include "Help.hpp"

#include <map>


static std::string quoted(const std::string& s) {
	json_t* j = json_string(s.c_str());
	char* out = json_dumps(j, JSON_ENCODE_ANY);
	std::string r = out ? out : "\"\"";
	std::free(out);
	json_decref(j);
	return r;
}

/** One entry, on one line, laid out as the database's own files are. */
static std::string entryLine(const std::vector<int>& ids, const std::string& name,
		const std::string& database) {
	std::string out = "{\"ids\": [";
	for (size_t i = 0; i < ids.size(); i++)
		out += (i ? ", " : "") + std::to_string(ids[i]);
	out += "]";
	if (!name.empty())
		out += ", \"name\": " + quoted(name);
	out += ", \"text\": \"\"";
	if (!database.empty())
		out += ", \"database\": " + quoted(database);
	return out + "}";
}

static const char* KIND_KEYS[HELP_KINDS] = {"inputs", "outputs", "params", "lights"};

/** What the module calls each of its controls of one kind, by number. Empty where the module
could not be made, and then the database's numbers are all there is to go on. */
static std::vector<std::string> namesOf(engine::Module* m, int kind) {
	std::vector<std::string> out;
	if (!m)
		return out;
	switch (kind) {
		case HELP_INPUT:
			for (engine::PortInfo* i : m->inputInfos)
				out.push_back(i ? i->name : "");
			break;
		case HELP_OUTPUT:
			for (engine::PortInfo* i : m->outputInfos)
				out.push_back(i ? i->name : "");
			break;
		case HELP_PARAM:
			for (engine::ParamQuantity* q : m->paramQuantities)
				out.push_back(q ? q->name : "");
			break;
		case HELP_LIGHT:
			for (engine::LightInfo* i : m->lightInfos)
				out.push_back(i ? i->name : "");
			break;
	}
	return out;
}

static std::string fileFor(plugin::Model* model, engine::Module* m) {
	const HelpModuleData db = helpDatabaseOnly(model->plugin->slug, model->slug);
	std::vector<std::string> out;
	out.push_back("{");
	out.push_back("  \"plugin\": " + quoted(model->plugin->slug) + ",");
	out.push_back("  \"module\": " + quoted(model->slug) + ",");
	out.push_back("  \"description\": \"\",");
	for (int k = 0; k < HELP_KINDS; k++) {
		const std::vector<std::string> names = namesOf(m, k);
		// AS MANY AS THE MODULE HAS, or as the database knows of where it could not be made.
		int count = (int) names.size();
		if (!m) {
			for (const auto& kv : db.items[k])
				count = std::max(count, kv.first + 1);
		}
		if (count == 0)
			continue;
		// ONE ENTRY PER SENTENCE, as the database groups them; every control the database says
		// nothing about gets an entry of its own, empty, to be written.
		std::vector<std::vector<int>> groups;
		std::vector<std::string> texts;
		std::map<std::string, size_t> seen;
		for (int id = 0; id < count; id++) {
			auto it = db.items[k].find(id);
			const std::string text = it != db.items[k].end() ? it->second.text : "";
			if (!text.empty() && seen.count(text)) {
				groups[seen[text]].push_back(id);
				continue;
			}
			if (!text.empty())
				seen[text] = groups.size();
			groups.push_back(std::vector<int>(1, id));
			texts.push_back(text);
		}
		out.push_back(std::string("  \"") + KIND_KEYS[k] + "\": [");
		for (size_t g = 0; g < groups.size(); g++) {
			const int first = groups[g][0];
			const std::string name = first < (int) names.size() ? names[first] : "";
			out.push_back("    " + entryLine(groups[g], name, texts[g])
				+ (g + 1 < groups.size() ? "," : ""));
		}
		out.push_back("  ],");
	}
	out.push_back("  \"notes\": [],");
	out.push_back("  \"menu\": [],");
	// THE DATABASE'S TEXT FOR THE MODULE AS A WHOLE, for reference, like each entry's.
	out.push_back("  \"database\": {");
	std::vector<std::string> ref;
	ref.push_back("    \"description\": " + quoted(db.description.text));
	auto list = [&](const std::vector<std::string>& v) {
		std::string s = "[";
		for (size_t i = 0; i < v.size(); i++)
			s += (i ? ", " : "") + quoted(v[i]);
		return s + "]";
	};
	ref.push_back("    \"notes\": " + list(db.notes));
	ref.push_back("    \"menu\": " + list(db.menu));
	for (size_t i = 0; i < ref.size(); i++)
		out.push_back(ref[i] + (i + 1 < ref.size() ? "," : ""));
	out.push_back("  }");
	out.push_back("}");
	std::string text;
	for (const std::string& l : out)
		text += l + "\n";
	return text;
}

std::string helpExport(plugin::Plugin* p, const std::string& folder) {
	if (!p)
		return "Nothing was exported.";
	system::createDirectories(folder);
	int wrote = 0, kept = 0, failed = 0;
	for (plugin::Model* model : p->models) {
		if (!model)
			continue;
		const std::string path = folder + "/" + model->slug + ".json";
		if (system::isFile(path)) {
			kept++;
			continue;
		}
		// THE MODULE ITSELF, MADE AND THROWN AWAY, because only a module knows how many
		// controls it has and what it calls them. Never added to the engine. A module that
		// cannot be made on its own is exported from the database's numbers instead.
		engine::Module* m = NULL;
		try {
			m = model->createModule();
		}
		catch (...) {
			m = NULL;
		}
		const std::string text = fileFor(model, m);
		delete m;
		if (FILE* f = std::fopen(path.c_str(), "wb")) {
			std::fwrite(text.data(), 1, text.size(), f);
			std::fclose(f);
			wrote++;
		}
		else {
			failed++;
		}
	}
	std::string said = string::f("Wrote %d help file%s for %s to %s.", wrote, wrote == 1 ? "" : "s",
		p->name.c_str(), folder.c_str());
	if (kept)
		said += string::f(" Left %d that were already there.", kept);
	if (failed)
		said += string::f(" Could not write %d.", failed);
	return said;
}
