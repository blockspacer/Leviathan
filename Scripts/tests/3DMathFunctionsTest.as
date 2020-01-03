// Tests 3d math related functions
bool TestAngleConversions()
{
    Radian rad = Degree(180);
    int valueDeg = int(rad.ValueInDegrees());
    if(valueDeg != 180) {
        // TODO: find a good way to run tests in as
        LOG_ERROR("Values don't match: " + formatInt(valueDeg) + " != " + formatInt(180));
        return false;
    }

    // This may not crash (that's the test)
    Quaternion quat = Quaternion(Float3(1, 0, 0), Degree(90));

    return true;
}
