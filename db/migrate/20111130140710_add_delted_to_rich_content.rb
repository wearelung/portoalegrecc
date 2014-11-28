class AddDeltedToRichContent < ActiveRecord::Migration
  def self.up
    add_column :rich_contents, :choosen, :integer, :default => 0
  end
  
  def self.down
    remove_column :rich_contents, :choosen
  end
end
