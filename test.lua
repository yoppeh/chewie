return {
    {
        name = "getenv",
        description = "Return the value of a specified environment variable",
        parameters = {{
            name = "varname",
            description = "The environment variable to read",
            type = "string",
            required = true
        },
        {
            name = "default",
            description = "The default value to return if the environment variable is not set",
            type = "string",
            choices = {"default", "empty", "nil"}
        }},
        code = function(varname, default)
            res = os.getenv(varname);
            if (res == fail) then
                return default
            end
            return res
        end
    },
    {
        name = "gettime",
        description = "Return the current time",
        code = function()
            return os.date()
        end
    }
}
