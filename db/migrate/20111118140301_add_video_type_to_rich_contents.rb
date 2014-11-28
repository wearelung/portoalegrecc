class AddVideoTypeToRichContents < ActiveRecord::Migration
  def self.up
  	add_column :rich_contents, :video_type, :string
  end

  def self.down
  	remove_column :rich_contents, :video_type
  end
end
