#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

class wxTextFile; // forward declaration

namespace LizardTool
{

// contains all the data needed for an instance of the Lizard game

struct Data
{
	struct Item
	{
		wxString name;
		int type;
		bool unsaved;

		virtual bool load(wxTextFile& f, wxString& err, Data* data) = 0;
		virtual bool save(wxTextFile& f, wxString& err) const = 0;
		virtual bool copy(const Item* src) = 0;
	};

	// 1k CHR page
	struct Chr : public Item
	{
		unsigned char data[16*4][8*8]; // 16 x 4 sprites each 8x8 in size

		mutable int export_index_cache; // filled on export for indexing

		Chr();
		virtual bool load(wxTextFile& f, wxString& err, Data* data);
		virtual bool save(wxTextFile& f, wxString& err) const;
		virtual bool copy(const Item* src);
	};

	// Palette
	struct Palette : public Item
	{
		unsigned char data[4];

		Palette();
		virtual bool load(wxTextFile& f, wxString& err, Data* data);
		virtual bool save(wxTextFile& f, wxString& err) const;
		virtual bool copy(const Item* src);
	};

	// Sprite
	struct Sprite : public Item
	{
		static const int MAX_SPRITE_TILES = 64;

		bool vpal;
		int bank;
		int count;
		int tile   [MAX_SPRITE_TILES];
		int palette[MAX_SPRITE_TILES];
		int x      [MAX_SPRITE_TILES];
		int y      [MAX_SPRITE_TILES];
		bool flip_x[MAX_SPRITE_TILES];
		bool flip_y[MAX_SPRITE_TILES];

		Chr* tool_chr[4];
		Palette* tool_palette[4];

		Sprite();
		virtual bool load(wxTextFile& f, wxString& err, Data* data);
		virtual bool save(wxTextFile& f, wxString& err) const;
		virtual bool copy(const Item* src);
	};

	struct Room : public Item
	{
		struct Dog
		{
			int type; // 0 = empty
			int x,y;
			unsigned char param;
			wxString note;

			mutable Sprite* cached_sprite; // used by room tool to map dogs to sprites faster
		};

		unsigned char nametable[64*30]; // 64x30 background tiles
		unsigned char attribute[32*15]; // 2x2 palette selection table

		Chr* chr[8]; // 8 1k CHR pages for background tiles
		Palette* palette[8]; // 8 palettes to use (palette 0 will be used by lizard anyway)

		int coord_x;
		int coord_y;
		bool recto;

		int music;
		int water;
		bool scrolling;
		Dog dog[16];

		Room* door[8];
		int door_x[8];
		int door_y[8];

		bool ship;

		Room();
		virtual bool load(wxTextFile& f, wxString& err, Data* data);
		virtual bool save(wxTextFile& f, wxString& err) const;
		virtual bool copy(const Item* src);

		// returns oob if outside of room
		bool collide_pixel(int x, int y, bool oob) const;
		bool collide_tile(int tx, int ty, bool oob) const;
	};

	wxString package_file;
	std::vector<Chr*    > chr;
	std::vector<Palette*> palette;
	std::vector<Sprite* > sprite;
	std::vector<Room*   > room;
	bool unsaved;
	wxString file_error;

	// temporary links for loading rooms
	struct LoadLink { Room* room; int door; wxString link; };
	std::vector<LoadLink> links;

	// clipboard for CHR tile and room tiles
	unsigned char chr_clipboard[8*8];
	unsigned char nmt_clipboard[2048];
	unsigned int nmt_clip_w;
	unsigned int nmt_clip_h;

	Data();
	~Data();
	void clear();
	// load/save/export are always relative to package_filename
	bool load();
	bool save();
	bool assign_coins_and_flags(int& coins, int& flags);
	bool validate();
	bool validate_extra(); // stand-alone validate, goes deep, does not export
	wxString password_build(unsigned short int zp_current_room, unsigned char zp_current_lizard) const;
	bool pack_room(Room* r, unsigned char (&buffer)[4096], unsigned int &packed_size);
	bool export_(unsigned char start_lizard);
	bool render_maps(bool underlay);
	bool render_sprites();
	const wxString& get_file_error() const; // error string if load/save/export fails

	enum ItemTypeEnum { ITEM_CHR=0, ITEM_PALETTE, ITEM_SPRITE, ITEM_ROOM, ITEM_TYPE_COUNT };

	unsigned int get_item_count(int t) const;
	Item* get_item(int t, unsigned int i);
	Item* find_item(int t, wxString name);
	int find_index(const Item* item);
	Room* find_room(int x, int y, bool recto);
	unsigned int sprite_export_index(Sprite* s); // index of sprite in exported data bank

	bool new_item(int t, const wxString& name);
	bool copy_item(int t, unsigned int i, const wxString& name);
	bool rename_item(int t, unsigned int i, const wxString& name);
	bool delete_item(int t, unsigned int i);
	bool swap_item(int t, unsigned int a, unsigned int b);
	wxString reference_item(int t, unsigned int i);

	static wxColour* nes_palette();
	static bool nes_palette_init;
	static wxColour nes_palette_data[64];

	// lizard combobox contents
	unsigned int lizard_count();
	const wxString* lizard_list();
	wxString* lizard_list_string;

	// music combobox contents
	int music_count();
	const wxString* music_list();
	wxString* music_list_string;

	// dog combobox contents
	int dog_count();
	const wxString* dog_list();
	wxString* dog_list_string;

	// dog sprite mapping
	void dog_sprite_validate_cache(); // internal utility for caching dog sprite mappings
	const Sprite* get_dog_sprite(const Room::Dog& dog);
	bool is_utility(int d); // utility dogs can be hidden from room renders
	Sprite empty_sprite;
	Sprite** dog_sprite_map;
	bool* dog_sprite_utility;
	bool dog_sprite_map_dirty;
};

} // namespace LizardTool
