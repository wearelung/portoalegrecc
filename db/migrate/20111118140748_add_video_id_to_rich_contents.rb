class AddVideoIdToRichContents < ActiveRecord::Migration
  def self.up
  	add_column :rich_contents, :video_id, :string
  end

  def self.down
  	remove_column :rich_contents, :video_id
  end
end
