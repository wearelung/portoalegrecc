class CreateRichContents < ActiveRecord::Migration
  def self.up
    create_table :rich_contents do |t|
      t.string :url
      t.integer :type
      t.integer :cause_id
      t.timestamps
    end
  end

  def self.down
    drop_table :rich_contents
  end
end
