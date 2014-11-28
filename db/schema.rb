# This file is auto-generated from the current state of the database. Instead of editing this file, 
# please use the migrations feature of Active Record to incrementally modify your database, and
# then regenerate this schema definition.
#
# Note that this schema.rb definition is the authoritative source for your database schema. If you need
# to create the application database on another system, you should be using db:schema:load, not running
# all the migrations from scratch. The latter is a flawed and unsustainable approach (the more migrations
# you'll amass, the slower it'll run and the greater likelihood for issues).
#
# It's strongly recommended to check this file into your version control system.

ActiveRecord::Schema.define(:version => 20120504135239) do

  create_table "categories", :force => true do |t|
    t.string   "name",        :null => false
    t.integer  "category_id"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

  create_table "causes", :force => true do |t|
    t.string   "author",                                                               :null => false
    t.string   "title",                                                                :null => false
    t.text     "abstract",                                                             :null => false
    t.decimal  "latitude",          :precision => 20, :scale => 17,                    :null => false
    t.decimal  "longitude",         :precision => 20, :scale => 17,                    :null => false
    t.string   "local",                                                                :null => false
    t.string   "district",                                                             :null => false
    t.boolean  "is_rejected",                                       :default => false, :null => false
    t.integer  "views",                                             :default => 0
    t.integer  "category_id"
    t.integer  "user_id"
    t.datetime "created_at"
    t.datetime "updated_at"
    t.string   "email"
    t.string   "phone_no"
    t.string   "cell_phone_no"
    t.integer  "submited",                                          :default => 0
    t.integer  "likes",                                             :default => 0
    t.datetime "last_likes_update"
    t.string   "protocol",                                          :default => "-1"
  end

  create_table "rich_contents", :force => true do |t|
    t.string   "url"
    t.integer  "kind"
    t.integer  "cause_id"
    t.datetime "created_at"
    t.datetime "updated_at"
    t.string   "photo_file_name"
    t.string   "photo_content_type"
    t.integer  "photo_file_size"
    t.datetime "photo_updated_at"
    t.string   "video_type"
    t.string   "video_id"
    t.integer  "choosen",            :default => 0
  end

  create_table "taggings", :force => true do |t|
    t.integer  "tag_id"
    t.integer  "taggable_id"
    t.integer  "tagger_id"
    t.string   "tagger_type"
    t.string   "taggable_type"
    t.string   "context"
    t.datetime "created_at"
  end

  add_index "taggings", ["tag_id"], :name => "index_taggings_on_tag_id"
  add_index "taggings", ["taggable_id", "taggable_type", "context"], :name => "index_taggings_on_taggable_id_and_taggable_type_and_context"

  create_table "tags", :force => true do |t|
    t.string "name"
  end

  create_table "users", :force => true do |t|
    t.string   "username",                         :null => false
    t.string   "password"
    t.integer  "level",             :default => 0, :null => false
    t.integer  "twitter_user_id"
    t.string   "google_email"
    t.string   "facebook_id"
    t.string   "name"
    t.string   "profile_image_url"
    t.datetime "created_at"
    t.datetime "updated_at"
    t.string   "location"
    t.string   "twitter_username"
    t.string   "email"
    t.datetime "last_sign_in"
  end

end
