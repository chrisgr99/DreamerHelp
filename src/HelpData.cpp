/** Where a module's help comes from — see design/help-database.md, and Help.hpp for the API.

THREE PLACES, IN ORDER, FOR EVERY ITEM SEPARATELY. The maker's own file in their plugin's `help`
folder; a user's file in the Rack user folder; and this plugin's database. They are read lowest
first and each later one writes over what it has text for, so whatever the maker wrote wins, a
user's text comes before ours, and ours covers the rest. An entry with empty text has said
nothing and writes over nothing.

READ ON THE FIRST QUESTION ABOUT A MODULE, AND KEPT. A click needs one module's files, three
small reads at most; reading four thousand at launch would cost seconds for nothing.
*/
#include "Help.hpp"

#include <map>
#include <memory>


static const char* SOURCE_NAMES[] = {"", "maker", "user", "database"};

const char* helpSourceName(HelpSource s) {
	return SOURCE_NAMES[s];
}

std::string helpMakerFile(const std::string& plugin, const std::string& model) {
	plugin::Plugin* p = plugin::getPlugin(plugin);
	if (!p)
		return "";
	return p->path + "/help/" + model + ".json";
}

std::string helpUserFolder(const std::string& plugin) {
	return asset::user("DreamerHelp/help/" + plugin);
}

std::string helpUserFile(const std::string& plugin, const std::string& model) {
	return helpUserFolder(plugin) + "/" + model + ".json";
}

std::string helpDatabaseFile(const std::string& plugin, const std::string& model) {
	return asset::plugin(pluginInstance, "data/help/" + plugin + "/" + model + ".json");
}


/** The keys a file lists its controls under, in HelpKind order. */
static const char* KIND_KEYS[HELP_KINDS] = {"inputs", "outputs", "params", "lights"};

/** Text, or empty for anything that is not a non-empty string. */
static std::string textOf(json_t* j) {
	return json_is_string(j) ? json_string_value(j) : "";
}

/** A list of strings, without the empty ones. */
static std::vector<std::string> linesOf(json_t* j) {
	std::vector<std::string> out;
	if (!json_is_array(j))
		return out;
	size_t i;
	json_t* v;
	json_array_foreach(j, i, v) {
		const std::string s = textOf(v);
		if (!s.empty())
			out.push_back(s);
	}
	return out;
}

/** One file, laid over what the sources before it said. A file that is not there is not a
problem; a file that is there and will not parse is, and the card says so rather than quietly
showing the next source instead — somebody editing it needs to know their words are not the ones
on the screen. */
static void layOver(HelpModuleData& d, const std::string& path, HelpSource from) {
	if (path.empty() || !system::isFile(path))
		return;
	json_error_t err;
	json_t* root = json_load_file(path.c_str(), 0, &err);
	if (!root || !json_is_object(root)) {
		d.problems.push_back(string::f("%s could not be read, at line %d, column %d: %s",
			path.c_str(), err.line, err.column, err.text));
		if (root)
			json_decref(root);
		return;
	}
	d.files[from] = path;

	const std::string description = textOf(json_object_get(root, "description"));
	if (!description.empty())
		d.description = HelpText{description, from};
	const std::vector<std::string> notes = linesOf(json_object_get(root, "notes"));
	if (!notes.empty()) {
		d.notes = notes;
		d.notesFrom = from;
	}
	const std::vector<std::string> menu = linesOf(json_object_get(root, "menu"));
	if (!menu.empty()) {
		d.menu = menu;
		d.menuFrom = from;
	}

	for (int k = 0; k < HELP_KINDS; k++) {
		json_t* list = json_object_get(root, KIND_KEYS[k]);
		if (!json_is_array(list))
			continue;
		size_t i;
		json_t* entry;
		json_array_foreach(list, i, entry) {
			const std::string text = textOf(json_object_get(entry, "text"));
			if (text.empty())
				continue;
			// ONE NUMBER OR A LIST OF THEM. The format says a list; a single number is what
			// somebody writing one entry by hand will type, and it means the same thing.
			json_t* ids = json_object_get(entry, "ids");
			std::vector<int> which;
			if (json_is_integer(ids))
				which.push_back((int) json_integer_value(ids));
			size_t n;
			json_t* id;
			json_array_foreach(ids, n, id) {
				if (json_is_integer(id))
					which.push_back((int) json_integer_value(id));
			}
			for (int at : which)
				d.items[k][at] = HelpText{text, from};
		}
	}

	// WHAT A JACK EXPECTS, per input and per output, by number.
	json_t* expects = json_object_get(root, "expects");
	for (int k = 0; k < 2; k++) {
		json_t* table = json_object_get(expects, KIND_KEYS[k]);
		const char* key;
		json_t* v;
		json_object_foreach(table, key, v) {
			const std::string text = textOf(v);
			if (!text.empty())
				d.expects[k][std::atoi(key)] = HelpText{text, from};
		}
	}
	json_decref(root);
}


static std::map<std::string, std::shared_ptr<HelpModuleData>> gCache;

const HelpModuleData& helpData(const std::string& plugin, const std::string& model) {
	const std::string key = plugin + "/" + model;
	auto it = gCache.find(key);
	if (it != gCache.end())
		return *it->second;
	auto d = std::make_shared<HelpModuleData>();
	layOver(*d, helpDatabaseFile(plugin, model), HELP_FROM_DATABASE);
	layOver(*d, helpUserFile(plugin, model), HELP_FROM_USER);
	layOver(*d, helpMakerFile(plugin, model), HELP_FROM_MAKER);
	gCache[key] = d;
	return *d;
}

void helpReload() {
	gCache.clear();
}


// ---- the database on its own, for the export -------------------------------------------------

HelpModuleData helpDatabaseOnly(const std::string& plugin, const std::string& model) {
	HelpModuleData d;
	layOver(d, helpDatabaseFile(plugin, model), HELP_FROM_DATABASE);
	return d;
}
