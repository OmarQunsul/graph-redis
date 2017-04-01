require 'redis'

redis = Redis.new

describe 'wrong type' do
  before do
    redis.flushdb
  end

  it "should return error while adding vertex to non-graph key" do
    redis.set("graph1", "value1")
    ret = redis.type('graph1')
    expect(ret).to eq('string')
    expect do
      redis.gvertex("graph1", "v1")
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end

  it "should return error while adding an edge to null key" do
    expect do
      redis.gedge("graph1", "v1", "v2", 10)
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end

  it "should return error while adding an edge to non-graph key" do
    redis.set("graph1", "value1")
    ret = redis.type('graph1')
    expect(ret).to eq('string')
    expect do
      redis.gedge("graph1", "v1", "v2", 10)
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end
end
