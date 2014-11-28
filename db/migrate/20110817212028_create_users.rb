class CreateUsers < ActiveRecord::Migration
  def self.up
    create_table :users do |t|
      t.string  :username,        :null => false
      t.string  :password
      t.integer :level,           :null => false, :default => 0
      t.integer :twitter_user_id
      t.string  :google_email
      t.string  :facebook_id
      t.string  :name
      t.string  :profile_image_url

      t.timestamps
    end
  end

  def self.down
    drop_table :users
  end
end
