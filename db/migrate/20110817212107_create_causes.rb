class CreateCauses < ActiveRecord::Migration
  def self.up
    create_table :causes do |t|
      t.string      :author,      :null => false
      t.string      :title,       :null => false
      t.text        :abstract,    :null => false
      t.decimal     :latitude,    :null => false
      t.decimal     :longitude,   :null => false
      t.string      :local,       :null => false
      t.string      :district,    :null => false
      t.boolean     :is_rejected, :null => false, :default => 0
      t.integer     :views,                       :default => 0
      t.belongs_to  :category
      t.belongs_to  :user

      t.timestamps
    end
  end

  def self.down
    drop_table :causes
  end
end
